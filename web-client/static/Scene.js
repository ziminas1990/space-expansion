// запустить корабль и посмотреть как он летит
//  у shape  в attrs есть rotation равное NaN ?
// сделать текстуру астероиду
// запушить в новосозданную ветку
class Scene {
  constructor(stage) {
    this.stage = stage
    this.view = new StageView()
    this.binding = new StageViewBinder(this.stage, this.view)
    this.asteroids_layer = new Konva.Layer();
    this.ships_layer = new Konva.Layer();
    this.stage.add(this.asteroids_layer);
    this.stage.add(this.ships_layer);
    this.shapes = new Map()
  }
// надо перенести методы поворота в подкласс shape
  update(items_container, now) {
    for (let item of items_container.items_stream()) {
      let item_key = item.strkey()
      let shape = this.shapes.get(item_key)
      if (shape == undefined) {
        shape = this.spawn_shape(item)
        this.shapes.set(item_key, shape)
        if (item.item_type == "ship") { 
         this.rotate_shape(item, shape);//  у shape  в attrs есть rotation равное NaN ?
        }
      } if (shape != null) {  // если оставить два If то срабатывает в первый раз оба условия. можно или написать if else или сделать два ифа и просчитать что будет срабатывать оба условия и всё основное оставить только во втором ифе 1 раз
        let predicted = item.predict_position(now);
        shape.position(predicted);
        if (item.item_type == "ship") {
          this.rotate_shape(item, shape);//  у shape  в attrs есть rotation равное NaN ?
        }
      }
    }
  }

  spawn_shape(item) {
    let shape = null
    if (item.item_type == "ship") {
      if (item.id.startsWith('Miner')) {
        shape = new Konva.Group({  
          offset: {               
            x: item.radius,     
            y: item.radius,    
          }
        })
        this.ships_layer.add(shape) 
        let corpus = new Konva.Circle({
          radius: item.radius,
          fill: "grey",
          stroke: 'rgb(72,72,72)',
          strokeWidth: 1,
         });
        let luck = new Konva.Circle({
          radius: item.radius/3,
          fill: 'grey',
          stroke: 'rgb(72,72,72)',
          strokeWidth: 1,
        });
        let mainingDevice = new Konva.Rect({
         x: - 1,
          width: 2,
          height: 5,
          fill: 'rgba(51, 51, 51)',
        });
        let wingL = new Konva.Line({
          points: [-item.radius/3, 0, -item.radius/3, item.radius*1.5, -item.radius/2  , item.radius*1.5, -item.radius, 0 ,-item.radius/3, 0 ],
          fill: 'grey',
          stroke: 'rgb(72,72,72)',
          strokeWidth: 1,
          closed: true,
          tension: 0.3,
        });
        let wingR = new Konva.Line({
          points: [item.radius/3, 0, item.radius/3, item.radius*1.5, item.radius/2  , item.radius*1.5, item.radius, 0 ,item.radius/3, 0 ],
          fill: 'grey',
          stroke: 'rgb(72,72,72)',
          strokeWidth: 1,
          closed: true,
          tension: 0.3,
        });
        shape.add(corpus, luck, mainingDevice, wingL, wingR);
      } else {
         shape = new Konva.Group({  
           offset: {               
             x: 0,     
             y: 0,     
           }
         })
         this.ships_layer.add(shape)
         let corpus = new Konva.Ring({
           innerRadius: item.radius/2,
           outerRadius: item.radius,
           fill: "grey",
           stroke: 'rgb(72,72,72)',
           strokeWidth: 1,
         });
         let luck = new Konva.Circle({
           radius: item.radius/4,
           fill: 'grey',
           stroke: 'rgb(72,72,72)',
           strokeWidth: 1,
         });
         let sunCorpus = new Konva.Ring({
           innerRadius: item.radius + item.radius/4,
           outerRadius: item.radius*1.5,
           fill: 'rgba(0,0,70)',
         });
         let outerlines = new Konva.Group({
           offset: {               
             x: 0,     
             y: 0,     
           }
         }).moveTo(this.ships_layer);
         let innerlines = new Konva.Group({
           offset: {               
             x: 0,     
             y: 0,     
           }
         }).moveTo(this.ships_layer);
         for (let i = 0; i < 360; i += 60) {
            let outerline = new Konva.Rect({
               width: item.radius/10,
               height: item.radius*1.5,
               fillLinearGradientStartPoint: { x: 0, y: 0 },
               fillLinearGradientEndPoint: { x: item.radius*1.5, y: item.radius*1.5 },
               fillLinearGradientColorStops: [0, 'black', 1, 'rgb(130,130,130)'],
            });
            outerline.rotate(i);
            outerlines.add(outerline);
         }
         for (let i = 30; i < 360; i += 60) {
            let innerline = new Konva.Rect({
               width: item.radius/10,
               height: item.radius*1.5,
               fillLinearGradientStartPoint: { x: 0, y: 0 },
               fillLinearGradientEndPoint: { x: item.radius*1.5, y: item.radius*1.5 },
               fillLinearGradientColorStops: [0, 'black', 1, 'rgb(80,80,80)'],
            });
            innerline.rotate(i);
            innerlines.add(innerline);
         }
         shape.add( innerlines, sunCorpus,  outerlines, corpus, luck,);
         let sunButteryRotation = new Konva.Animation(function(){
            let angleDiff = 0.05;
            innerlines.rotate(angleDiff);
            outerlines.rotate(-angleDiff);
         }, this.ships_layer);
         sunButteryRotation.start();
      }
    } else if (item.item_type == "asteroid") { 
      shape = new Konva.Circle({
        radius: item.radius,
        fill: 'rgba(50,50,50)',
      });
      this.asteroids_layer.add(shape);
    }
    return shape
  }

  rotate_shape(item, shape){
      
      let last_angle = shape.getAbsoluteRotation();
      // console.log(last_angle); // вначале 6 раз по -0 потом Nan
      let angle = 0;
      let animationRotation = new Konva.Animation(function (frame) {
        if (item.position.vx == 0 && item.position.vy > 0) {
          if (angle == Math.abs(last_angle)) {
            animationRotation.stop();         
          }
          let new_angle = calculate_angle(angle, last_angle);
          shape.rotation(new_angle);
          //  корабль должен быть на 6 часов
        } else if (item.position.vx < 0 && item.position.vy > 0) {
          angle =  Math.atan(item.position.vx/item.position.vy)*180 / Math.PI;
          if (angle == last_angle) {
            animationRotation.stop();         
          }
          let new_angle = calculate_angle(angle, last_angle)
          shape.rotation(new_angle);
          //  корабль должен быть примерно на 7.5 часов
        } else if (item.position.vx < 0 && item.position.vy == 0) {
          if (angle == last_angle) {
            animationRotation.stop();         
          }
          let new_angle = calculate_angle(angle, last_angle);
          shape.rotation(new_angle);
          //  корабль должен быть на 9 часов
        } else if (item.position.vx < 0 && item.position.vy < 0) {
          angle =  Math.atan(item.position.vy/item.position.vx)*180 / Math.PI;
          if (angle == last_angle) {
            animationRotation.stop();         
          }
         let new_angle = calculate_angle(angle, last_angle);
         shape.rotation(new_angle);
         
          //  корабль должен быть примерно на 10.5 часов
        } else if (item.position.vx == 0 && item.position.vy < 0) {
          if (angle == last_angle) {
            animationRotation.stop();         
          }
         let new_angle = calculate_angle(angle, last_angle);
         shape.rotate(new_angle);
          //  корабль должен быть на 12 часов
        } else if (item.position.vx > 0 && item.position.vy < 0) {
          angle =  Math.atan(item.position.vy/item.position.vx)*180 / Math.PI;
          if (angle == last_angle) {
            animationRotation.stop();         
          }
         
          let new_angle = calculate_angle(angle, last_angle);
          shape.rotate( - new_angle);
          //  корабль должен быть примерно  на 2.5 часов
        } else if (item.position.vx > 0 && item.position.vy == 0) {
          if (angle == last_angle) {
            animationRotation.stop();         
          }
          
          let new_angle = calculate_angle(angle, last_angle);
          shape.rotate( - new_angle);
          //  корабль должен быть на 3 часов
        }  else if (item.position.vx > 0 && item.position.vy > 0) {
          angle =  Math.atan(item.position.vx/item.position.vy)*180 / Math.PI;
          if (angle == last_angle) {
            animationRotation.stop();         
          }
         let new_angle = calculate_angle(angle, last_angle)
         shape.rotate( - new_angle);
          //  корабль должен быть примерно на 4.5 часов
        } 

        function calculate_angle (angle, last_angle) {
         if (last_angle <= 0 && angle <= 0) {
            let delta_angle = Math.ceil(Math.abs(last_angle) - Math.abs(angle));
            return delta_angle;
         } else if (last_angle >= 0 && angle <= 0) {
            let delta_angle = - Math.ceil((Math.abs(last_angle) - Math.abs(angle)));
            return delta_angle;
         } else if (last_angle >= 0 && angle >= 0) {
            let delta_angle = Math.ceil(Math.abs(angle - last_angle));
            return delta_angle;
         } else if (last_angle <= 0 && angle <= 0) {
            let delta_angle = Math.ceil(Math.abs(angle) + Math.abs(last_angle));
            return delta_angle;
         }
        }
      }, this.ships_layer);
         animationRotation.start();
  }

}


