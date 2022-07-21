function set_status(status) {
  document.getElementById("status").innerHTML = status
}

function runApp() {
  timestamp = {
   "server_us": 0,
   "local_ms": 0
  }

  predict_now = function () {
    return timestamp.server_us + (Date.now() - timestamp.local_ms) * 1000
  }
  
  // Create components:
  connection = new Connection()
  items = new ItemsContainer()
  let stage = new Konva.Stage({
    container: 'plot',
    width: window.innerWidth,
    height: window.innerHeight,
  });
  let scene = new Scene(stage)

  // Create a web-socket connection
  set_status("Connecting...")
  connection.open(
    window.location.host,
    function (event) {
      document.getElementById("status").innerHTML = "Connection established";
    }
  )
   let item = undefined;
   connection.addEventListener('message', function (event) { 
   const update = JSON.parse(event.data);  
   timestamp.server_us = update.ts;             
   timestamp.local_ms = Date.now()               
   set_status(JSON.stringify(timestamp)); 
   if(item === undefined) {                
      for (let object_info of update.items) {
         item = new Item(object_info);          
         item.timestamp = timestamp.server_us;  
         items.update_item(item);
   }} 
   else {                                                              
      for (let object_info of update.items) {   
         //?console.log("hi from messege")
         let update_item = new Item(object_info);
         update_item.update();
      }}               
   });

  animation = new Konva.Animation(function (frame) {
    scene.update(items, predict_now())
  });
  animation.start()
}
