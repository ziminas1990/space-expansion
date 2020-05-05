# Изучение состава астероида
В текущей версии игры астероиды являются источниками трёх ресурсов:
1. металлы;
2. лёд;
3. силикаты.

При этом соотношение этих ресурсов у разных астероидов может быть различным. Чтобы исследовать примерный состав астероида, нужно использовать модули класса **"AsteroidScanner"**.

## Интерфейс IAsteroidScanner
Все модули класса "AsteroidScanner" реализуют интерфейс IAsteroidScanner:
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

## Как получить параметры сканера?
Чтобы получить параметры сканера, нужно отправить запрос **specification_req**. В ответ поступит следующее сообщение **specification**:
```protobuf
message Specification {
  uint32 max_distance     = 1;
  uint32 scanning_time_ms = 2;
}
```
, где:
  * **max_distance** - максимальная дистанция до астероида, выражена в метрах;
  * **scanning_time_ms ** - время, требуемое на сканирование 1 гектара поверхности (100 x 100 метров).

## Как сканировать астероид?
Для того, чтобы просканировать астероид, необходимо:
1. обнаружить астероид; например, используя модуль, реализующий интерфейс *ICelestialScanner*;
2. подлететь к нему на дистанцию, не превышающую **max_distance**;
3. запустить сканирование, отправив команду **scan_asteroid**, в котором передаётся идентификатор астероида;
4. до тех пор, пока не поступит ответ, держаться от астероида на дистанции, не превышающей *max_distance*.

В ответ сервер отправит сообщение **scanning_status** с одним из следующих статусов:
  - **IN_PROGRESS** - сканирование запущено, ожидайте результатов;
  - **SCANNER_BUSY** - сканирование уже идёт (было запущено ранее);
  - **ASTEROID_TOO_FAR** - указанный астероид слишком далеко (либо вообще не существует).

Если сканирование прошло успешно, модуль отправит ответ **scanning_finished**:
```protobuf
message ScanResult {
  uint32 asteroid_id       = 1;
  double weight            = 2;
  double metals_percent    = 3;
  double ice_percent       = 4;
  double silicates_percent = 5;
}
```
, где:
  * **asteroid_id** - уникальный идентификатор астероида;
  * **weight** - масса астероида, в килограммах;
  * **metals_percent** - процент металлов;
  * **ice_percent** - процент льда;
  * **silicates_percent**- процент силикатов.

Если сканирование астероида не удалось, модуль отправит ещё одного сообщение **scanning_status** с указанием одной из следующих причин:
  - **ASTEROID_TOO_FAR** - в процессе сканирования корабль отдалился от астероида слишком далеко, либо астероид перестал существовать (разрашуен?).
