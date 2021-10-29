from enum import Enum


class BaseStatus(Enum):
    SUCCESS = "success"

    FAILED_TO_SEND_REQUEST = "failed to send request"
    RESPONSE_TIMEOUT = "response timeout"
    UNEXPECTED_RESPONSE = "unexpected response"
    CHANNEL_CLOSED = "channel closed"
    CANCELED = "operation canceled"

    def is_success(self):
        return self == BaseStatus.SUCCESS

    def is_timeout(self):
        return self == BaseStatus.RESPONSE_TIMEOUT

    def is_network_problem(self):
        return self == BaseStatus.FAILED_TO_SEND_REQUEST

    def is_closed(self):
        return self == BaseStatus.CHANNEL_CLOSED

    def is_canceled(self):
        return self == BaseStatus.CANCELED

    def is_unexpected_response(self):
        return self == BaseStatus.UNEXPECTED_RESPONSE
