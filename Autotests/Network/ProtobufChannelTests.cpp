#include <gtest/gtest.h>
#include <google/protobuf/util/message_differencer.h>
#include <Network/ProtobufChannel.h>
#include <Protocol.pb.h>
#include <Autotests/TestUtils/ProtobufSyncPipe.h>

namespace autotests
{

class ProtobufChannelTests : public ::testing::Test
{
public:

  void SetUp() override
  {
    m_pChannel        = std::make_shared<network::PlayerChannel>();
    m_pMockedTerminal = std::make_shared<ProtobufSyncPipe>();

    m_pChannel->attachToTerminal(m_pMockedTerminal);

    m_pMockedTerminal->setEnviromentProceeder(
          [this]() { m_pChannel->handleBufferedMessages(); }
    );
  }

protected:
  void sendMessage(spex::Message const& message);
  bool expect(spex::Message const& expected);

  void createSomeMessages(std::vector<spex::Message>& out);

protected:
  network::PlayerChannelPtr m_pChannel;
  ProtobufSyncPipePtr       m_pMockedTerminal;
};

void ProtobufChannelTests::sendMessage(spex::Message const& message)
{
  std::string data;
  message.SerializeToString(&data);
  m_pChannel->onMessageReceived(0, network::BinaryMessage(data.data(), data.size()));
}

bool ProtobufChannelTests::expect(const spex::Message &expected)
{
  spex::Message receivedMessage;
  return m_pMockedTerminal->waitAny(0, receivedMessage)
      && google::protobuf::util::MessageDifferencer::Equals(expected, receivedMessage);
}

void ProtobufChannelTests::createSomeMessages(std::vector<spex::Message> &out)
{
  {
    spex::Message message;
    spex::IAccessPanel::LoginRequest *pReq =
        message.mutable_accesspanel()->mutable_login();
    pReq->set_ip("1.2.3.4.");
    pReq->set_login("admin");
    pReq->set_password("fojiolly");
    pReq->set_port(2233);
    out.push_back(std::move(message));
  }
  {
    spex::Message message;
    message.mutable_accesspanel()->set_access_rejected("test");
    out.push_back(std::move(message));
  }
  {
    spex::Message message;
    message.mutable_accesspanel()->set_access_granted(12345);
    out.push_back(std::move(message));
  }
}


TEST_F(ProtobufChannelTests, SingleMessage)
{
  std::string sReason = "test";
  {
    spex::Message message;
    message.mutable_accesspanel()->set_access_rejected(sReason);
    sendMessage(message);
  }
  {
    spex::IAccessPanel message;
    ASSERT_TRUE(m_pMockedTerminal->wait(0, message));
    EXPECT_EQ(spex::IAccessPanel::kAccessRejected, message.choice_case());
    EXPECT_EQ(sReason, message.access_rejected());
  }
}


TEST_F(ProtobufChannelTests, LotsOfMessage)
{
  std::vector<spex::Message> messages;
  createSomeMessages(messages);

  for (size_t i = 0; i < 5; ++i) {
    spex::Message const& message = messages[i % messages.size()];
    sendMessage(message);
    ASSERT_TRUE(expect(message));
  }
}


TEST_F(ProtobufChannelTests, LotsOfMessageSimultaneously)
{
  std::vector<spex::Message> messages;
  createSomeMessages(messages);

  for (size_t i = 0; i < 5; ++i) {
    spex::Message const& message = messages[i % messages.size()];
    sendMessage(message);
  }

  for (size_t i = 0; i < 5; ++i) {
    spex::Message const& message = messages[i % messages.size()];
    ASSERT_TRUE(expect(message));
  }
}

} // namespace autotests
