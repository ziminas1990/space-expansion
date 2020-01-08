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
    double weight            = 2;
    double metals_percent    = 3;
    double ice_percent       = 4;
    double silicates_percent = 5;
  }
  message ScanFailed {}


  oneof choice {
    GetSpecification get_specification = 1;
    ScanRequest      scan_request      = 2;
    
    Specification    specification     = 21;
    ScanResult       scan_result       = 22;
    ScanFailed       scan_failed       = 23;
  }
}
```

## Как получить параметры сканера?
Чтобы получить параметры сканера, нужно отправить запрос **GetSpecification**. В ответ поступит следующее сообщение **Specification**:
```protobuf
message Specification {
  uint32 max_distance    = 1;
  uint32 scanning_time_k = 2;
}
```
, где:
  * **max_distance** - максимальная дистанция до астероида, выражена в метрах;
  * **scanning_time_k ** - время, требуемое на сканирование 1 гектара поверхности (100 x 100 метров).

## Как сканировать астероид?
Для того, чтобы просканировать астероид, необходимо:
1. обнаружить астероид; например, используя модуль, реализующий интерфейс *ICelestialScanner*;
2. подлететь к нему на дистанцию, не превышающую **max_distance**;
3. запустить сканирование, отправив команду **scan_request**;
4. до тех пор, пока не поступит ответ, держаться от астероида на дистанции, не превышающей *max_distance*.

Запрос **scan_request**:
```protobuf
message ScanRequest {
  uint32 asteroid_id = 1;
}
```
, где:
  * **asteroid_id** - уникальный идентификатор астероида; можно получить, например, сканируя астероиды через интерфейс *ICelestialScanner*.

Ответ на запрос имеет вид:
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

В случае, если сканирование астероида не удалось, сканер отправляет сообщение **ScanFailed**. Оно может быть отправлено по следующим причинам:
  * уже запущена другая задача сканирования;
  * расстояние до астероида превышает максимально допустимое *max_distance*;
  * в процессе сканирования корабль отдалился от астероида на дистанцию, превышающую *max_distance*.

