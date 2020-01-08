
# Модуль "AsteroidMiner"
Данный интерфейс позволяет управлять добычей ресурсов на астероидах.
Модули, реализующий данный интерфейс, имеет следующие параметры:
  - **max_distance** - максимально допустимая дистанция до астероида, при которой возможна добыча;
  - **cycle_time_ms** - цикл добычи, или как часто модуль будет перекладывать всё добытое в контейнер и высылать уведомление (report) об объёмах добытого ресурса;
  - **yield_per_cycle** - масса вещества астероида (в килограммах), которая обрабатывается в течении одного цикла.

## Описание интерфейса IAsteroidMiner
Все модули класса "AsteroidMiner" реализуют интерфейс **IAsteroidScanner**:
```protobuf
message IAsteroidMiner {

  enum Status {
    SUCCESS               = 0;
    INTERNAL_ERROR        = 1;
    ASTEROID_DOESNT_EXIST = 2;
    MINER_IS_BUSY         = 3;
    MINER_IS_IDLE         = 4;
    ASTEROID_TOO_FAR      = 5;
    NO_SPACE_AVALIABLE    = 6;
  }

  message Specification {
    uint32 max_distance    = 1;
    uint32 cycle_time_ms   = 2;
    uint32 yeild_pre_cycle = 3;
  }

  message MiningTask {
    uint32       asteroid_id = 1;
    ResourceType resource    = 2;
  }

  oneof choice {
    bool          specification_req = 1;
    MiningTask    start_mining      = 2;
    bool          stop_mining       = 3;

    Specification specification       = 21;
    Status        start_mining_status = 22;
    ResourceItem  mining_report       = 23;
    Status        stop_mining_status  = 24;
    Status        on_error            = 25;
  }
}
```

## Как получить параметры бура?
Чтобы получить параметры бура, нужно отправить запрос **specification_req **. В ответ поступит сообщение **specification**, содержащее значения параметров устройства:
```protobuf
message Specification {
  uint32 max_distance    = 1;
  uint32 cycle_time_ms   = 2;
  uint32 yeild_pre_cycle = 3;
}
```
## Как запустить добычу?
Для того, чтобы запустить добычу на астероиде, необходимо:
1. обнаружить астероид и определить его уникальный идентификатор (с помощью модуля AsteroidScanner).
2. приблизиться к астероиду на расстояние не более чем **max_distance** метров;
3. отправить команду **start_mining**, указав идентификатор астероида и тип ресурса, который требуется добывать.

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

Так же, устройство может отправить сообщение об ошибке **on_error** в случае, если процесс добычи был прерван. Сообщение может содержать следующие причины прерывания:
  * **ASTEROID_DOESNT_EXIST** - астероид больше не существует (был уничтожен?);
  * **ASTEROID_TOO_FAR** - корабль удалился от астероида на расстояние, превышающее максимально допустимое;
  * **NO_SPACE_AVALIABLE** - в контейнере закончилось место.

## Остановка добычи
Для того, чтобы остановить добычу ресурса, необходимо отправить команду **stop_mining**. Эта команда имеет тип **bool**, однако её значение игнорируется.  
В ответ на команду устройство отправит сообщение **stop_mining_status**, которое может содержать одно из следующих значений:
  * **SUCCESS** - процесс добычи остановлен;
  * **MINER_IS_IDLE** - команда проигнорирована, т.к. процесс добычи не был запущен.

