# Протокол взаимодействия
Протокол взаимодействия реализован на базе библиотеки [Protobuf](https://developers.google.com/protocol-buffers) от Google. Весь интерфейс описан в виде одного файла [Protocol.proto](https://github.com/ziminas1990/space-expansion/blob/master/Protocol.proto). Файл написан с использованием спецификации языка ["Proto3 language guide"](https://developers.google.com/protocol-buffers/docs/proto3).  
Для обмена сообщениями между сервером и клиентом используется протокол UDP, без каких-либо дополнительных заголовков для обнаружения/восстановления потерь. Возможно в будущем такой заголовок появится.  
Задачи реализации обмена по UDP и [интеграции библиотеки protobuf](https://developers.google.com/protocol-buffers/docs/tutorials) в Ваш проект не будут рассматриваться в данной статье. Далее будем считать, что Вы уже умеете отправлять и принимать protobuf-сообщения и сосредоточимся на рассмотрении логики обмена сообщениями.

Клиент и сервер могут обмениваться между собой исключительно сообщениями **Message**:
```protobuf
message Message {
  uint32 tunnelId = 1;

  oneof choice {
    // All possible interfaces are listed here
    Message           encapsulated     = 2;
    IAccessPanel      accessPanel      = 3;
    ICommutator       commutator       = 4;
    IShip             ship             = 5;
    INavigation       navigation       = 6;
    IEngine           engine           = 7;
    ICelestialScanner celestialScanner = 8;
    IAsteroidScanner  asteroid_scanner = 9;
    // и другие интерфейсы....
  }
}
```
Как видно, данное сообщение является контейнером для любого другого сообщения, в т.ч. для вложенного (encapsulated) сообщения **Message**. По мере добавления новых модулей, их сообщения так же будут добавляться в **Message**. Каждое из возможных вложенных сообщений будет рассмотрено ниже.

Кроме того, что Message является контейнером для других сообщений, он имеет одно дополнительное поле **tunnelId**. Это номер туннеля или номер виртуального канала. Оно так же будет рассмотрено ниже, при обсуждении интерфейса **ICommutator**.

В дальнейшем, мы будем считать что **любое** сообщение, отправленное на сервер или полученное от сервера, была "упаковано" в сообщение **Message**.  
Например, если нужно отправить на сервер сообщение **LoginRequest**, которое является частью интерфейса **IAccessPanel** (о нём речь пойдёт ниже), то необходимо:
  - создать сообщение Message;
  - в сообщение Message вложить сообщение IAccessPanel;
  - в сообщение IAccessPanel вложить сообщение LoginRequest.
 
 Т.е. сообщение будет иметь вид:
 ![encapsulate-example](https://i.ibb.co/Ss1yQKT/encapsulation-example.png)
