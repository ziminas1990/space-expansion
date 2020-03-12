
# Протокол взаимодействия
Протокол взаимодействия реализован на базе библиотеки [Protobuf](https://developers.google.com/protocol-buffers) от Google. Весь интерфейс описан в виде одного файла [Protocol.proto](https://github.com/ziminas1990/space-expansion/blob/master/Protocol.proto). Файл написан с использованием спецификации языка ["Proto3 language guide"](https://developers.google.com/protocol-buffers/docs/proto3).  
Для обмена сообщениями между сервером и клиентом используется протокол UDP, без каких-либо дополнительных заголовков для обнаружения/восстановления потерь. Возможно в будущем такой заголовок появится.  
Задачи реализации обмена по UDP и [интеграции библиотеки protobuf](https://developers.google.com/protocol-buffers/docs/tutorials) в Ваш проект не будут рассматриваться в данной статье. Далее будем считать, что Вы уже умеете отправлять и принимать protobuf-сообщения и сосредоточимся на рассмотрении логики самого протокола.

Клиент и сервер могут обмениваться между собой **исключительно** сообщениями **Message**:
```protobuf
message Message {
  uint32 tunnelId = 1;

  oneof choice {
    // All possible interfaces are listed here
    Message            encapsulated       = 2;
    IAccessPanel       accessPanel        = 3;
    ICommutator        commutator         = 4;
    IShip              ship               = 5;
    INavigation        navigation         = 6;
    IEngine            engine             = 7;
    ICelestialScanner  celestial_scanner  = 8;
    IAsteroidScanner   asteroid_scanner   = 9;
    // etc...
  }
}
```
Как видно, данное сообщение является контейнером для любого другого сообщения, в т.ч. для вложенного (encapsulated) сообщения **Message**. По мере добавления новых модулей, их сообщения  будут добавлены в **Message**. Каждое из возможных вложенных сообщений будет рассмотрено ниже.

Кроме того что Message является контейнером для других сообщений, он имеет одно дополнительное поле **tunnelId**. Это номер туннеля или номер виртуального канала. Оно так же будет рассмотрено ниже, при обсуждении интерфейса **ICommutator**.

В дальнейшем, мы будем считать что **любое** сообщение, отправленное на сервер или полученное от сервера, была "упаковано" в сообщение **Message**.  
Например, если нужно отправить на сервер сообщение **LoginRequest**, которое является частью интерфейса **IAccessPanel** (о нём речь пойдёт ниже), то необходимо:
  - создать сообщение Message;
  - в сообщение Message вложить сообщение IAccessPanel;
  - в сообщение IAccessPanel вложить сообщение LoginRequest.
 
 Т.е. сообщение будет иметь вид:
 ![encapsulate-example](https://i.ibb.co/Ss1yQKT/encapsulation-example.png)

## Интерфейсы модулей
Каждый модуль реализует один или несколько **интерфейсов**. Каждый интерфейс - это сообщение верхнего уровня, начинающееся с буквы "I". Например:
  - **IEngine** - интерфейс управления двигателем;
  - **IAsteroidScanner** - интерфейс модуля, сканирующего астероиды.

Для обеспечения единообразия, интерфейсы всех модулей построены по одному и тому же принципу. Пример интерфейса:
```protobuf
message IAsteroidScanner {

  enum Status {
    IN_PROGRESS        = 0;
    SCANNER_BUSY       = 1;
    ASTEROID_TOO_FAR   = 2;
  }

  message Specification {
    uint32 max_distance     = 1;
    uint32 scanning_time_ms = 2;
  }
  
  message ScanResult {
    uint32 asteroid_id       = 1;
    double weight            = 2;
    double metals_percent    = 3;
    double ice_percent       = 4;
    double silicates_percent = 5;
  }

  oneof choice {
    bool   specification_req = 1;
    uint32 scan_asteroid     = 2;
    
    Specification specification     = 21;
    Status        scanning_status   = 22;
    ScanResult    scanning_finished = 23;
  }
}
```

В интерфейсе могут содержаться:
  - перечисление (enum) **Status**, в котором собраны в кучу различные коды ошибок или уведомлений, которые могут быть использованы в других сообщениях данного интерфейса;
  - вложенные сообщения, такие как **Specification** или **ScanResult** - они описывают структуру тех сообщений интерфейса, которые имеют более одного поля;
  - поле **choice**, в котором перечислены все возможные сообщения интерфейса.

В поле **choice** перечислены как команды, которые клиент отправляет серверу, так и ответы/извещения, поступающие от сервера клиенту.

В тех случаях, если команда или ответ имеют только одно поле, то из соображений эффективности и простоты для него не будет создано отдельное вложенное сообщение. Вместо этого, тип такой команды/ответа будет совпадать с типом его единственного поля. В данном примере, в интерфейсе есть команда **scan_asteroid**. Она имеет тип **uint32**, т.к. содержит только одно поле - уникальный идентификатор астероида, который необходимо изучить. Конечно, можно было бы сделать так:
```protobuf3
message IAsteroidScanner {
  // ...
  message ScanAsteroid {
    uint32 asteroid_id = 1;
  }
  //...
  oneof choice {
    bool         specification_req = 1;
    ScanAsteroid scan_asteroid     = 2;

    //...
  }
}
```
Но это только бы усложнило и замедлило как клиентский, так и серверный код.

## Общие типы
Есть набор типов, которые являются общими и могут использоваться в разных интерфейсах. Более того, использование в интерфейсах общих типов поощряется при разработке новых интерфейсов. Все эти типы приведены в начале proto-файла в секции "Common Types":
```protobuf
message NamesList {
  uint32          left  = 1;
  repeated string names = 2;
}

enum ResourceType {
  RESOURCE_UNKNOWN   = 0;
  RESOURCE_METALS    = 1;
  RESOURCE_SILICATES = 2;
  RESOURCE_ICE       = 3;
  RESOURCE_LABOR     = 101;
}

message ResourceItem {
  ResourceType type  = 1;
  double       amount = 2;
}

message Property {
  string            name   = 1;
  string            value  = 2;
  repeated Property nested = 3;
}

message Blueprint {
  string                name       = 1;
  repeated Property     properties = 2;
  repeated ResourceItem expenses   = 3;
}

message Position {
  double x   = 1;
  double y   = 2;
  double vx  = 4;
  double vy  = 5;
}
```
Рассмотрим каждый из типов:
  - **NamesList** - сообщение для передачи списка строк (имён):
	  - **names** - массив имён;
	  - **left** - сколько имён из списка ещё не передано; в последнем сообщении поле будет иметь нулевое значение;
  - **ResourceType** - перечисление возможных типов ресурсов в игре;
  - **ResourceItem** - ресурс (тип + количество);
  - **Property** - описание некоторого свойства; имеет древовидную структуру, т.к. может содержать в себе вложенные свойства;
	  - **name** - имя свойства;
	  - **value** - значение свойства;
	  - **nested** - вложенные свойства (если они имеются);
  - **Blueprint** - описание некоторого чертежа, по которому может быть изготовлен игровой предмет;
	  - **name** - имя чертежа;
	  - **preperties** - свойства предмета, изготовленного по чертежу;
	  - **expenses**- стоимость изготовления предмета;
  - **Position** - позиция в пространстве; содержит в себе как координаты (x, y), так и текущий вектор скорости {vx, vy}.