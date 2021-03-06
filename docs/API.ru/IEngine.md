
# Двигатели
## Что есть двигатель?
Двигатели - это модули класса **"Engine"**. Такие модули позволяют создавать вектор тяги - силу, действующую на объект, на который установлен двигатель.

В текущей версии двигатель сильно упрощён:
  - имеет единственный параметр - **максимальная сила тяги**, выраженная в Ньютонах;
  - может изменять свой вектор тяги **моментально** как по силе, так и по направлению;
  - запас топлива неограничен;
  - топливо имеет нулевую массу, т.е. работе двигателя не влияет на массу корабля.

С развитием игры и повышением уровня сложности, все эти упрощения будут постепенно устраняться.

## Интерфейс IEngine
Основной интерфейс управления двигателем - **IEngine**:
```protobuf
message IEngine
{
  message Specification {
    uint32 max_thrust = 1;
  }

  message ChangeThrust {
    double x           = 1;
    double y           = 2;
    uint32 thrust      = 4;
    uint32 duration_ms = 5;
  }

  message CurrentThrust {
    double x      = 1;
    double y      = 2;
    uint32 thrust = 4;
  }

  oneof choice {
    bool         specification_req = 1;
    ChangeThrust change_thrust     = 2;
    bool         thrust_req        = 3;

    Specification specification = 21;
    CurrentThrust thrust        = 22;
  }
}
```

## Как получить параметры двигателя?
Чтобы получить параметры двигателя, нужно отправить запрос **specification_req**. Значение поля игнорируется. В ответ двигатель отправит сообщение **specification** с типом **Specification**, в котором передаются следующие параметры:

| Параметр   | Ед. изм. | Возможные значения | Описание                                     |
|------------|----------|--------------------|----------------------------------------------|
| max_thrust | H        | >= 0               | Максимально допустимое значение вектора тяги |

## Как управлять вектором тяги двигателя?
Для того, чтобы установить вектор тяги двигателя, нужно отправить команду **change_thrust**, имеющую тип **ChangeThrust**:
```protobuf
message ChangeThrust {
  double x           = 1;
  double y           = 2;
  uint32 thrust      = 4;
  uint32 duration_ms = 5;
}
```
, где:
  - **x** и **y** задают **направление** вектора тяги; модуль вектора игнорируется;
  -  **thrust** определяет силу вектора тяги, выраженную в Ньютонах; если указанная сила превышает максимально возможную для данного двигателя, то ~~двигатель взрывается~~ будет выставлена максимально возможная;
  - **duration_ms** - время работы двигателя в миллисекундах; 

Для того, чтобы получить текущее значение вектора тяги, нужно отправить запрос **thrust_req** с произвольным значением. В ответ поступит сообщение **thrust** типа **CurrentThrust**, где:
```protobuf
message CurrentThrust {
  double x      = 1;
  double y      = 2;
  uint32 thrust = 4;
}
```
, где:
  - **x, y** - координаты нормированного вектора тяги; 
  - **thrust**- сила вектора тяги (в Ньютонах).
