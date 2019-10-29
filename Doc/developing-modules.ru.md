

Здесь будет описан алгоритм добавления нового модуля в проект. Под модулем понимается оборудование, которое устанавливается на корабль. Руководство будет написано на примере модуля AsteroidScanner.

## Добавление интерфейса модуля в Protocol.proto файл
Первый шаг - продумать интерфейс для управления модулем и описать его в виде protobuf-сообщений. Для этого, в **Protocol.proto** добавим новый интерфейс **IAsteroidScanner**. В этом интерфейсе должны содержаться все сообщения, которыми могут обмениваться клиент и сервер. Например, так:
```
message IAsteroidScanner {

  message GetSpecification {}
  message Specification {
    uint32 max_distance    = 1;
    uint32 scanning_time_k = 2;
  }
  
  message ScanRequest {
    uint32 asteroid_id = 1;
  }
  message ScanResult {
    uint32 asteroid_id       = 1;
    double weight            = 2
    double metals_percent    = 3;
    double ice_percent       = 4;
    double silicates_percent = 5;
  }
  message ScanFailed {}


  oneof choice {
    GetSpecification get_specification = 1;
    ScanRequest      scan_request      = 2;
    
    ScanResult       scan_result       = 21;
    ScanFailed       scan_failed       = 22;
  }
}
```

Далее этот интерфейс необходимо добавить в верхнеуровневое сообщение Message примерно таким образом:
```
message Message {
  uint32 tunnelId = 1;

  oneof choice {
    // All possible interfaces are listed here
    // <...>
    IAsteroidScanner  asteroid_scanner = 9;
    // <...>
  }
}
```

## Поддержка интерфейса на уровне BaseModule
Чтобы поддержать новый интерфейс на уровне **BaseModule**, необходимо:
1. Добавить protected virtual-функцию для обработки сообщений нового интерфейса. Например, так:
```cpp
protected:
  virtual void handleAsteroidScannerMessage(uint32_t, spex::IAsteroidScanner const&) {}
```
2. сделать вызов виртуальной функции из **BaseModule::handleMessage()**:
```cpp
void BaseModule::handleMessage(uint32_t nSessionId, spex::Message const& message)
{
  switch(message.choice_case()) {
    // ...
    case spex::Message::kAsteroidScanner: {
      handleAsteroidScannerMessage(nSessionId, message.asteroid_scanner());
      return;
    }
    // ...
  }
}
```

## Подготовка файлов
В директории **Modules** необходимо создать поддиректорию одноимённую с названием модуля. Например "Modules/AsteroidScanner".
В данной директории создаём следующие файлы:
  - .cpp и .h - файлы с реализацией логики модуля; например AsteroidScanner.h и AsteroidScanner.cpp;
  - .h - файл с объявлением менеджера для модуля; например, AsteroidScannerManager.h.
Если менеджер будет нестандартным, то для него придётся создать и .cpp - файл.

В заголовочном файле **AsteroidScanner.h** объявляем в namespace'е **modules** класс модуля AsteroidScanner, унаследовав его от:
  - **BaseModule** - базовый класс для любых модулей; объявлен в файле <Modules/BaseModule.h>;
  - **utils::GlobalContainer\<AsteroidScanner\>** - контейнер для всех инстансов данного модуля; объявлен в файле <Utils/GlobalContainer.h>.

В .cpp-файле **вне** namespace'а необходимо разметить макрос
```cpp
DECLARE_GLOBAL_CONTAINER_CPP(modules::AsteroidScanner);
```

## Наследование BaseLogic
### Конструктор
Поскольку класс модуля был унаследован от BaseModule и от utils::GlobalContainer, необходимо в конструкторе расположить:
  - вызов конструктора BaseLogic, передав ему тип модуля в виде строки;
  - регистрацию объекта в GLobalContainer'е.
Поэтому в **минимально комплектации** конструктор будет выглядеть так:
```cpp
AsteroidScanner::AsteroidScanner()
  : BaseModule("AsteroidScanner")
{
  GlobalContainer<AsteroidScanner>::registerSelf(this);
}
```
В конструктор класса можно передать основные параметры модуля.

### Загрузка начального состояния
Если модуль может иметь какое-то начальное состояние, то необходимо переопределить функцию
```cpp
bool BaseModule::loadState(YAML::Node const& source);
```
Начальное состояние хранится в виде JSON-объекта и модуль должен уметь его считывать. Если считать состояние не удалось, либо оно некорректное, функция должна вернуть false.  
Модуль **AsteroidScanner** не имеет начального состояния, поэтому ему не требуется переопределять эту функцию. А вот, например, модуль двигателя **Engine** имеет начальное состояние, в котором описываются текущий вектор тяги и количество топлива в баках. Поэтому ему потребуется переопределить данную функцию.

При переопределении функции **loadStat** важно в первую очередь вызвать её реализацию для BaseModule, например так:
```cpp
bool Engine::loadState(YAML::Node const& source)
{
  if (!BaseModule::loadState(source))
    return false;
  // Ваш код считывания состояния
  return true;
}
```

### Обработка сообщений
Для обработки сообщений необходимо переопределить protected-функцию
```cpp
void BaseModule::handleAsteroidScannerMessage(uint32_t, spex::IAsteroidScanner const&);
```
, которая была добавлена в BaseModule в предыдущем шаге.

Если команда может быть обработана "здесь и сейчас", то достаточно обработать её и отправить ответ с помощью функции
```cpp
bool BaseModule::sendToClient(uint32_t nSessionId, spex::Message const& message)
```
Так же, для удобства можно добавить в BaseModule **специализацию** функции sendToClient и пользоваться ей:
```cpp
bool BaseModule::sendToClient(uint32_t nSessionId, spex::IAsteroidScanner const& message) const;
```

### Выполнение операций, растянутых в игровом времени
Если на обработку команд модулю требуется какое-то игровое время, то необходимо переопределить public-функцию
```cpp
void BaseModule::proceed(uint32_t nIntervalUs);
```
Она будет периодически вызываться игровым движком и ей будет передаваться количество микросекунд **игрового** времени, которое прошло с момента последнего вызова данной функции. Например, выполнение команды сканирования астероида требует определённое игровое время, а значит AsteroidScanner должен переопределять эту функцию. Есть команды, которые могут быть обработаны в тот же момент, когда они были получены и, если интерфейс модуля состоит **только** из таких команд, то функцию **proceed()** переопределять **НЕ** нужно.

Чтобы игровой движок начал вызывать функцию proceed(), необходимо перевести модуль в **активное состояние**. Для этого достаточно вызвать функцию
```cpp
void BaseModule::switchToActiveState();
```
После этого, игровой движок будет вызывать функцию proceed() для тех пор, пока модуль не перейдёт в неактивное состояние. Чтобы перевести модуль в неактивное состояние, нужно вызвать функцию
```cpp
void BaseModule::switchToIdleState();
```

## Получение информации об окружении
Понятно, что модуль существует не в вакууме (не факт, кстати) и для реализации его логики необходимо получать информацию о текущем состоянии игрового мира.

### Получение информации о корабле
Любой модуль устанавливается на корабль (или станцию или другую платформу). Платформу, на которой установлен модуль, можно получить через вызов:
```cpp
ships::Ship*       BaseModule::getPlatform();
ships::Ship const* BaseModule::getPlatform() const;
```
У платформы, в свою очередь, можно запросить такие данные, как, например, текущее положение в пространстве и вектор скорости.

### Получение информации об объектах игрового мира
Объекты игрового мира хранятся в **utils::GlobalContainer**. Это шаблонный класс и параметром шаблона является тип объекта.
Например:
  - **utils::GlobalContainer\<Asteroid\>** - массив всех астероидов в игровом мире;
  - **utils::GlobalContainer\<PhysicalObject\>** - массив всех физических объектов;
  - **utils::GlobalContainer\<Ship\>** - массив всех кораблей;
  - **utils::GlobalContainer\<Engine\>** - массив всех модулей "Engine";
  - и многие другие.

Так же нужно понимать, что поскольку класс **Asteroid** является наследником класса **PhysicalObject**, то объекты класс Asteroid будут присутствовать как в контейнере utils::GlobalContainer\<Asteroid\>, так и в контейнере utils::GlobalContainer\<PhysicalObject\>.

Класс **utils::GlobalContainer** предоставляет несколько **статических** функций для получения доступа к объектам:
  - ```uint32_t TotalInstancies() ``` - возвращает общее количество объектов данного типа;
  - ```Inheriter* Instance(uint32_t nInstanceId)``` - возвращает объект с индексом nInstanceId;
  - ```std::vector<Inheriter*> const& getAllInstancies()``` - возвращает все объекты данного типа.

По сути, это массив, поэтому скорость доступа к элементам - O(1).

## Реализация логики модуля
### Многопоточность
Логика модуля (обработка сообщений и proceed'ы) будет вызываться в многопоточной среде. Однако, игровой движок гарантирует, что в каждый момент времени будет многопоточно выполняться только логика **одинаковых** модулей. Иначе говоря, когда выполняется логика AsteroidScanner'ов, логика остальных модулей будет ожидать.
Зная это можно написать такую реализацию логики, которая не будет требовать синхронизации потоков. Например, логика AsteroidScanner'а будет выполнять над астероидами только операции чтения, без операций записи. А значит, все модули могут обращаться к астероидам на чтение без какой-либо синхронизации между собой.
Логику модулей необходимо стремится реализовать таким образом, что бы исключать или сводить к минимуму ситуации, когда один поток может быть заблокирован другим потоком.

### 
