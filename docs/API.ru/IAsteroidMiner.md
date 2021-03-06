# Модуль "AsteroidMiner"
Данный интерфейс позволяет управлять добычей ресурсов на астероидах.
Модули, реализующий данный интерфейс, имеет следующие параметры:
  - **max_distance** - максимально допустимая дистанция до астероида, при которой возможна добыча;
  - **cycle_time_ms** - цикл добычи, или как часто модуль будет перекладывать всё добытое в контейнер и высылать уведомление (report) об объёмах добытого ресурса;
  - **yield_per_cycle** - масса вещества астероида (в килограммах), которая обрабатывается в течении одного цикла.

## Описание интерфейса IAsteroidMiner
Все модули класса "AsteroidMiner" реализуют интерфейс **IAsteroidMiner**:
```protobuf
message IAsteroidMiner {

  enum Status {
    SUCCESS               = 0;
    INTERNAL_ERROR        = 1;
    ASTEROID_DOESNT_EXIST = 2;
    MINER_IS_BUSY         = 3;
    MINER_IS_IDLE         = 4;
    ASTEROID_TOO_FAR      = 5;
    NO_SPACE_AVAILABLE    = 6;
    NOT_BOUND_TO_CARGO    = 7;
    INTERRUPTED_BY_USER   = 8;
  }

  message Specification {
    uint32 max_distance    = 1;
    uint32 cycle_time_ms   = 2;
    uint32 yield_per_cycle = 3;
  }

  message MiningTask {
    uint32       asteroid_id = 1;
    ResourceType resource    = 2;
  }

  oneof choice {
    bool          specification_req = 1;
    string        bind_to_cargo     = 2;
    MiningTask    start_mining      = 3;
    bool          stop_mining       = 4;

    Specification specification        = 21;
    Status        bind_to_cargo_status = 22;
    Status        start_mining_status  = 23;
    ResourceItem  mining_report        = 24;
    Status        mining_is_stopped    = 25;
    Status        stop_mining_status   = 26;
  }

}
```
## Два канала?
При работе с данным модулем настоятельно рекомендуется открывать к нему два туннеля: один для команды **start_mining**, а второй - для всех остальных команд.
Это связано с тем, что команда **start_mining** имеет неопределённое время завершения (пока процесс не будет прерван). В то же время, отправлять по одному и тому же каналу команду на фоне выполнения другой команды - очень плохая идея, которая сильно усложнит обработку ответов.

## Как получить параметры бура?
Чтобы получить параметры бура, нужно отправить запрос **specification_req **. В ответ поступит сообщение **specification**, содержащее значения параметров устройства:
```protobuf
message Specification {
  uint32 max_distance    = 1;
  uint32 cycle_time_ms   = 2;
  uint32 yeild_pre_cycle = 3;
}
```

## Как подключить бур к контейнеру?
Чтобы добыча ресурсов была возможна, бур должен быть подключен к одному из контейнеров, расположенных на корабле. Изначально бур не подключен ни к одному из них, поэтому добыча ресурсов невозможна. Чтобы подключить бур к контейнеру, необходимо отправить команду **bind_to_cargo**. Тело команды - это строка, в которой указано имя контейнера.

В ответ поступит сообщение **bind_to_cargo_status**, в котором передаётся статус выполнения команды:
  * **SUCCESS** - модуль успешно подключен к контейнеру;
  * **NOT_BINT_TO_CARGO** - не удалось подключить майнер к контейнеру; возможно, что контейнер с таким именем не установлен на корабле.

Модуль может быть переподключен к другому контейнеру в процессе добычи.

## Как запустить добычу?
Для того, чтобы запустить добычу на астероиде, необходимо:
1. подключить бур к контейнеру;
2. обнаружить астероид и определить его уникальный идентификатор (с помощью модуля AsteroidScanner).
3. приблизиться к астероиду на расстояние не более чем **max_distance** метров;
4. отправить команду **start_mining**, указав идентификатор астероида и тип ресурса, который требуется добывать.

Команда **start_mining** имеет следующий формат:
```protobuf
message MiningTask {
  uint32       asteroid_id = 1;
  ResourceType resource    = 2;
}
```
, где:
  * **asteroid_id** - уникальный идентификатор астероида;
  * **resource** - тип ресурса, которые необходимо добывать.

В ответ на команду должно поступить сообщение **start_mining_status **, которое указывает статус выполнения команды:
  * **SUCCESS** - разработка астероида запущена;
  * **MINER_IS_BUSY** - добыча уже запущена;
  * **ASTEROID_DOESNT_EXIST** - астероида с указанным asteroid_id не существует;
  * **ASTEROID_TOO_FAR** - астероид слишком далеко.

## Мониторинг процесса добычи
В процессе добычи, устройство периодически отправляет отчёты (report) после каждого цикла. Отчёт передаётся как сообщение **mining_report**, которое имеет тип **ResourceItem**. В нём указан тип добытого ресурса и его масса.

Так же, устройство может отправить сообщение **mining_is_stopped** в случае, если процесс добычи был прерван. Сообщение может содержать следующие причины прерывания:
  * **INTERRUPTED_BY_USER** - процесс прерван пользователем (командой **stop_mining**)
  * **ASTEROID_DOESNT_EXIST** - астероид больше не существует (был уничтожен?);
  * **ASTEROID_TOO_FAR** - корабль удалился от астероида на расстояние, превышающее максимально допустимое;
  * **NO_SPACE_AVALIABLE** - в контейнере закончилось место.

## Остановка добычи
Для того, чтобы остановить добычу ресурса, необходимо отправить команду **stop_mining**. Эта команда имеет тип **bool**, однако её значение игнорируется.  
В ответ на команду устройство отправит сообщение **stop_mining_status**, которое может содержать одно из следующих значений:
  * **SUCCESS** - процесс добычи остановлен;
  * **MINER_IS_IDLE** - команда проигнорирована, т.к. процесс добычи не был запущен.

Кроме того, по каналу, в котором была отправлена команда **start_mining**, будет передано сообщение **mining_is_stopped** со статусом **INTERRUPTED_BY_USER**. Поэтому важно чтобы команды **start_mining** и **stop_mining** были отправлены устройству в разных каналах.

