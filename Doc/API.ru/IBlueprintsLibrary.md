# Интерфейс IBlueprintLibrary
Данный интерфейс позволяет игроку получить список доступных blueprint'ов и их характеристики, такие как стоимость производство объекта, либо свойства произведённого объекта.

## Описание интерфейса
```protobuf
message IBlueprintsLibrary {
  enum Status {
    SUCCESS             = 0;
    INTERNAL_ERROR      = 1;
    BLUEPRINT_NOT_FOUND = 2;
  }
  
  oneof choice {
    string    blueprints_list_req = 1;
    string    blueprint_req       = 2;
    
    NamesList blueprints_list     = 20;
    Blueprint blueprint           = 21;
    Status    blueprint_fail      = 22;
  }
}
```

## Запрос списка blueprint'ов
Чтобы получить список доступных игроку blueprint'ов, необходимо отправить запрос **blueprints_list_req**. Данный запрос является строкой, которая может быть использована как фильтр результата. Если в качестве фильтра указать пустую строку, то хранилище выдаст список имён всех имеющихся blueprint'ов. Если указать непустой фильтр, то будет выдан список из тех blueprint'ов, имя которых начинается с указанной строки.

В ответ на данный запрос поступит одно или несколько сообщений **blueprints_list**, которые имеют формат **NamesList** (см. [Общие сведения о протоколе](API.ru/protocol-general.md)).

Например, допустим что у игрока имеются следующие blueprint'ы:
```
Engine/Nuclear
Engine/Chemical
Engine/Antic-Nordic-Engine
ResourceContainer/Huge
ResourceContainer/Small
```
Тогда, если отправить запрос **blueprints_list_req** со строкой "Engine", будут возвращены только первые три имени.

## Запрос blueprint'а
Чтобы запросить подробную информацию о чертеже, нужно отправить команду  **blueprint_req**, в которой передаётся **полное** имя чертежа. Например "Engine/Nuclear". В ответ поступит сообщение **blueprint**, имеющее тип **Blueprint**:
```protobuf
message Blueprint {
  string                name       = 1;
  repeated Property     properties = 2;
  repeated ResourceItem expenses   = 3;
}
```
, где:
  - **name** - имя чертежа (совпадает с именем в команде);
  - **properties** - параметры объекта, который будет создан по данному чертежу;
  - **expenses** - затраты ресурсов, необходимые для производства данного объекта.
