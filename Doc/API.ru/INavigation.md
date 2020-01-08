# Интерфейс INavigation
У объекта, реализующего данный интерфейс, можно запросить информацию о его текущем местоположении и скорости.
```protobuf
message INavigation {

  message GetPosition {}
  message GetPositionResponse {
    double x   = 1;
    double y   = 2;
    double vx  = 4;
    double vy  = 5;
  }
  
  oneof choice {
    GetPosition positionRequest = 1;
    
    GetPositionResponse positionResponse = 2;
  }
}
```

Интерфейс имеет единственный запрос **GetPosition**. Если объект реализует данный интерфейс, то в ответ на запрос он отправит сообщение **GetPositionResponse**, в котором будут указаны текущее местоположение объекта (x, y), и его скорость (vx, vy).
