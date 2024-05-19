// Frogger Game Implementation
// Author: Shresth Mishra


import "./style.css";
import { fromEvent,interval,merge,Observable } from 'rxjs';
import { map,filter,flatMap,takeUntil,scan,mergeMap } from 'rxjs/operators';



function main() {

  /**
    * MoveFrog is a class that is created by the observeKey observable upon the triggering
      of specific key events. 
    * MoveFrog & Tick dictate changes in the state of the program.
  */
  class MoveFrog { constructor(public readonly directionX:number, public readonly directionY:number) {} }  
  class Tick { constructor(public readonly elapsed:number) {} }

  //The following types enables us to strictly define the allowed scope of observable events
  //Event type defines the allowed events for the observeKey observable
  type Event = 'keydown' | 'keyup'
  //Key type defines the allowed keys for the observeKey observable
  type Key = 'ArrowLeft' | 'ArrowRight' | 'ArrowUp' | 'ArrowDown'
  //ViewType type defines the allowed objects views when defining the objects view types
  type ViewType = 'frog' | 'car' | 'log' | 'river'

  /* 
    Implementation is based on the FRP Asteroids code.
    observeKey creates observable streams based on different 
    keys & events and maps these to create different classes, which
    then modify the state of the game changing the view.
  */
  const observeKey = <T>(eventName:Event, k:Key, result:()=>T)=>
    fromEvent<KeyboardEvent>(document,eventName)
      .pipe(
        filter(({code})=>code === k),
        filter(({repeat})=>!repeat),
        map(result)),
  /**These are the different input streams which map to different classes
   * They have been defines to enable the movement of the frog using the keyboard */
  startMoveLeft = observeKey('keydown','ArrowLeft',()=>new MoveFrog(-100,0)),
  startMoveRight = observeKey('keydown','ArrowRight',()=>new MoveFrog(100,0)),
  startMoveUp = observeKey('keydown','ArrowUp',()=>new MoveFrog(0,-100)),
  startMoveDown = observeKey('keydown','ArrowDown',()=>new MoveFrog(0,100)),
  stopMoveLeft = observeKey('keyup','ArrowLeft',()=>new MoveFrog(0,0)),
  stopMoveRight = observeKey('keyup','ArrowRight',()=>new MoveFrog(0,0)),
  stopMoveUp = observeKey('keyup','ArrowUp',()=>new MoveFrog(0,0)),
  stopMoveDown = observeKey('keyup','ArrowDown',()=>new MoveFrog(0,0))
  
  /** The purpose of defining the types below is so that we can define 
   * the different game mechanics used throughout our program
   * In order to maintian purity in the code all the types have been defines as ReadOnly
   * Defining the types as ReadOnly ensures that it is not possible to mutate the type values
   * This enables us to create new states for the types
  */ 

  // the Car type defines the constraints for the car object
  type Car = Readonly<{coordX:number, coordY:number, height:number, width:number}>
  // the River type defines the constraints for the river object
  type River = Readonly<{coordX:number, coordY:number, height:number, width:number}>
  // the Frog type defines the constraints for the frog object
  type Frog = Readonly<{coordX:number, coordY:number, height:number, width:number}>
  // the Log type defines the constraints for the log object
  type Log = Readonly<{coordX:number, coordY:number, height:number, width:number}>
  // the ObjectId type defines the constraints for each object
  type ObjectId = Readonly<{id:string,createTime:number}>
  
  // the Body type is defined as a generic type based on all the other types defined above
  interface IBody extends Car,River,Frog,Log, ObjectId {
    viewType: ViewType
  }
  type Body = Readonly<IBody>

  // the "State" type is used to define all the elements required for each state
  type State = Readonly<{
    time:number,
    frog:Body,
    cars:ReadonlyArray<Body>,
    river:ReadonlyArray<Body>,
    log:ReadonlyArray<Body>,
    score:number,
    gameOver:boolean
  }>

  // All the constansts that may be used in the program are defined below
  const 
    Constants = {
      CarWidth: 100,
      CarHeight: 100,
      FrogHeight: 100,
      FrogWidth: 100,
      CarCount: 4,
      RiverCount: 2,
      RiverWidth: 700,
      RiverHeight: 100,
      LogWidth: 200,
      LogHeight: 100,
      LogCount: 2,
      StartTime: 0,
      LogMovement: [-1,2],
      CarMovement: [1,-3,-2,2]
    } as const
  
  /**Since all the objects in the game are designed to be polygons,
   *A generic functions createPolygon has been created to create a new 
   *object based on its desiered parameters
  */
  const createPolygon = (viewType :ViewType)=> (oid:number)=> (time:number)=> 
  (width:number)=> (height:number)=> (inputCoordX: number)=> (inputCoordY: number)=> <Body>{
      createTime: time,
      id: viewType+oid,
      width: width,
      height:height,
      viewType: viewType,
      coordX: inputCoordX,
      coordY: inputCoordY
  }
  
  /**The following code defines all the objects used in the game
   *The objects are created using the createPolygon function 
    */
  const
  cars = [...Array(Constants.CarCount)].map((_,i)=>createPolygon("car")(i)
  (Constants.StartTime)(Constants.CarWidth)((Constants.CarHeight))(200+i*100)(300+i*100)),
  river = [...Array(Constants.RiverCount)].map((_,i)=>createPolygon("river")(i)
  (Constants.StartTime)(Constants.RiverWidth)(Constants.RiverHeight)(0)(100 + i*100)),
  logs = [...Array(Constants.LogCount)].map((_,i)=>createPolygon("log")(i)
  (Constants.StartTime)(Constants.LogWidth)(Constants.LogHeight)(400)(100 + i*100)),
  createFrog = createPolygon("frog")(0)(0)(100)(100)
  
  /**The initialState is a constant and is an immutable data structure
   * It is used to define the intial state of the program
   *initialState maintains the functional style of programming as it is never mutatated. 
   */
  const initialState:State = {
    time:0,
    frog:createFrog(300)(700),
    cars:cars,
    river:river,
    log:logs,
    score:0,
    gameOver:false
  }

  /**torusWrap function from FRP Astriod implemented
   * the original function has been modified for this implementation
  */
  const
  frogTorusWrapX = (coord: number) => {
    return coord < 0 ? 0 : coord > 600 ? 600 : coord},
  torusWrapY = (coord: number) => {
    return coord < 0 ? 0 : coord > 700 ? 700 : coord},
  torusWrapX = (coord: number) => {
    return coord < 0 ? 700 : coord >= 700 ? 0 : coord}
  
  /**
   * handleCollisions is used to check the collision between the frog and the car
   * or when the frog falls into the river
   * @param s is the state that is being passed into the function
   * @returns it returns the new state of the game, with a new gameOver
   */
  const handleCollisions = (s:State) => {
    const
      collided = ([a,b]:[Body,Body]) => {
        return a.coordX < b.coordX + b.width &&
               a.coordX + a.width > b.coordX &&
               a.coordY < b.coordY + b.height &&
               a.height + a.coordY > b.coordY},
      frogCarCollision = s.cars.filter(r=>collided([s.frog,r])).length > 0,
      riverCollided = ([a,b,c,d]:[Body,Body,Body,Body]) => 
      (a.coordY) === b.coordY && !(collided([a,c])||collided([a,d])),
      frogRiverCollision = s.river.filter(r=>riverCollided([s.frog,r,s.log[0],s.log[1]])).length > 0
      return <State> {...s,
        gameOver: frogRiverCollision || frogCarCollision
      }
  }

  /**
   * the tick function is called after every time step to update the state of
   * the cars,logs and score is updated to their correct vlue and position
   * @param s is the state that is being passed into the function
   * @param elapsed refers to the current time
   * @returns the handleCollisions function is called and 
   * the current state is passed to it
   */
  const tick = (s:State,elapsed:number) => {
    return handleCollisions({...s, 
      frog: createFrog(frogTorusWrapX(s.frog.coordX))(torusWrapY(s.frog.coordY)),
      time:elapsed,
      /**the cars and logs keep moving depending upon thier speed as defined in the constansts
       * and thier x coordinate value is incremented.
       * The cars and the logs are placed depending on the y coordinate values 
       */
      cars: [...Array(Constants.CarCount)].map((_,i)=>createPolygon("car")(i)
      (Constants.StartTime)(Constants.CarWidth)((Constants.CarHeight))
      (torusWrapX((s.cars[i].coordX)+Constants.CarMovement[i]))((300+i*100))),
      log: [...Array(Constants.LogCount)].map((_,i)=>createPolygon("log")(i)
      (Constants.StartTime)(Constants.LogWidth)(Constants.LogHeight)
      (torusWrapX((s.log[i].coordX)+Constants.LogMovement[i]))(100 + i*100)),
      //the score is determined depending on the frog's current row(y-coordinate)
      score: s.frog.coordY == 600 ? 1
           : s.frog.coordY == 500 ? 2
           : s.frog.coordY == 400 ? 3
           : s.frog.coordY == 300 ? 4
           : s.frog.coordY == 200 ? 6
           : s.frog.coordY == 100 ? 8
           : s.frog.coordY == 0   ? 12 
           : 0
    })
  }
  /**
   * The reduceState function is based on the FRP Asteroids example
   * It is a purely functional  function which takes in a state and class based on the gameEvent Observable. 
   * This function is used in to encapsulate all the possible transformations of state in a function to maintain an
   * accumulated state of our game without using any mutable variables or non-functional styles.
   * @param s is the state that is being passed into the function
   * @param e refers to the MoveFrog and Tick classes
   * @returns either a new location for the frog or calls the tick function
   */
  const reduceState = (s:State, e:MoveFrog|Tick) =>
    e instanceof MoveFrog ? {...s,
     frog: createFrog(s.frog.coordX + e.directionX)(s.frog.coordY + e.directionY)
    }: tick(s, e.elapsed)
  
  /**
   * Based on the FRP Asteroids
   * The updateView function is used to "view" the game.
   * It takes in the current state of the program and modify's the HTML objects.
   * The objects in the games are being transformed and updated.
   * @param s is the state that is being passed into the function
   */
  function updateView(s: State){
    const
    svg = document.getElementById("svgCanvas")!,
    frog = document.getElementById("frog")!,
    score = document.getElementById("score")!,
    updateBodyView = (b:Body) =>{
      function createBodyView() {
        const v = document.createElementNS(svg.namespaceURI, "rect")!;
        v.setAttribute('id', b.id)
        v.classList.add(b.viewType)
        svg.appendChild(v)
        return v;
      }
      const v = document.getElementById(b.id) || createBodyView();
      v.setAttribute('x', String(b.coordX))
      v.setAttribute('y', String(b.coordY))
      v.setAttribute('height', String(b.height))
      v.setAttribute('width', String(b.width))
    };
    s.cars.forEach(updateBodyView);
    s.river.forEach(updateBodyView);
    s.log.forEach(updateBodyView);
    frog.setAttribute('transform', `translate(${s.frog.coordX},${s.frog.coordY})`);
    score.innerHTML = "Score: " + s.score.toString();
    
    /**
     * The game over is handled here
     * Once gameOver is true, the unsubscribe() is used to end the game
     * Next Game Over is displayed
     */
    if(s.gameOver) {
      subscription.unsubscribe();
      const v = document.createElementNS(svg.namespaceURI, "text")!;
      v.setAttribute('x', '350');
      v.setAttribute('y', '400');
      v.setAttribute('fill', 'white');
      v.textContent = "Game Over"
      svg.appendChild(v);
    }
  }
  
  /** 
   *subscription merges gameClock with all the other moves into a single stream and
   *calls scan to accumulate the current state with the one based on the current event
   *An accumulated state is created starting with the initial game state and with each new event,
   *it is transformed and accumulated to a state which is then passed to updateView
   */
   const 
   gameClock = interval(10).pipe(
    map(elapsed=>new Tick(elapsed))),
    subscription = 
    merge<Tick|MoveFrog>(gameClock, startMoveLeft,startMoveRight,startMoveUp,startMoveDown,
    stopMoveLeft,stopMoveRight,stopMoveUp,stopMoveDown)
    .pipe(scan(reduceState,initialState))
    .subscribe(updateView)
  
}



// The following simply runs your main function on window load.  Make sure to leaåve it in place.
if (typeof window !== "undefined") {
  window.onload = () => {
    main();
  };
}
