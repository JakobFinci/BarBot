// Stepper Motor Library
#include <AccelStepper.h>
// Libraries for User Interface (i2c LCD)
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// User interface setup
LiquidCrystal_I2C lcd(0x26,16,3); 

// State LEDs
const int RED = 39;
const int YELLOW = 41;
const int GREEN = 43;
// Mode LEDs
const int BLUE = 45;
const int WHITE = 47;
// Pump Motors (4)
const int dirPin = 23;
const int stepPin = 2;
const int dir2Pin = 25;
const int step2Pin = 3;
const int dir3Pin = 25;
const int step3Pin = 4;
const int dir4Pin = 27;
const int step4Pin = 5;
//Expendable code for up to 6 motors
/* 
 * const int dir5Pin = 29;
 * const int step5Pin = 6;
 * const int dir6Pin = 32;
 * const int step6Pin = 7;
*/
// Motor Parameters
const int max_speed = 800;
const int acceleration = 400;
const int mit = 1;
const int decel_time = (max_speed / acceleration)*1000;
//motor driver enables
const int ena1 = 22;
const int ena2 = 24;
// Buttons
const int SELECT = 49;
const int CONFIRM = 51;
const int RESET = 53;
// Selection Potentiometer
const int POTSELECT = 1;

// FOR DEMO DAY: Pump 1 - Soda Water, Pump 2 - Citrus Gin, Pump 3 - Herbal Gin, Pump 4 - Honey

// Lists of Drink Strings
const int num_of_drinks = 8;
const int num_of_motors = 4;
/*
const char* drink_list[num_of_drinks] = {
  "NULL","Citrus Gin","Herbal Gin","Citrus Gin Soda","Herbal Gin Soda","Citrus Bee's Knees","Herbal Bee's Knees"
  };
long drink_array[num_of_drinks][num_of_motors] = {
  {0,0,0,0},{0,10000,0,0},{0,0,10000,0},{60000,10000,0,0},{60000,0,10000,0},{0,10000,0,20000},{0,0,10000,20000}
};
*/

// Demo Day Alt
const char* drink_list[num_of_drinks] = {
  "NULL","Soda Water","Citrus Gin","Herbal Gin","Citrus Gin Soda","Herbal Gin Soda","Citrus Bee's Knees","Herbal Bee's Knees"
  };
long drink_array[num_of_drinks][num_of_motors] = {
  {0,0,0,0},{40000,0,0,0},{0,20000,0,0},{0,0,10000,0},{45000,20000,0,0},{45000,0,10000,0},{0,20000,0,20000},{0,0,10000,20000}
};

/*Calibration Numbers
 * 1/2 oz = 8000;
 * 1 oz = 14000;
 * 1.5 oz = 18000;
 * 2 oz = 260000
 */
long pump_durs[num_of_motors];

// State timings
uint32_t blink_time;
uint32_t dispense_time;
uint16_t BLINK_INT = 500;
uint32_t sel_time;
uint16_t sel_count;
uint32_t dis_time;
uint16_t dis_count;
// Maintainance timings
uint32_t clean_time;
uint32_t setup_time;
// Pump durations during setup mode
long int cleandur = 90000;
long int setupdur = 9000;

// Choice instantiation
int temp_choice;
int temp_choice_raw;
int new_temp_choice;
int new_temp_choice_raw;
int choice_raw;
int drink_choice;
int prior_choice; 

// pump initialization
AccelStepper pump1 = AccelStepper(mit, stepPin, dirPin);
AccelStepper pump2 = AccelStepper(mit, step2Pin, dir2Pin);
AccelStepper pump3 = AccelStepper(mit, step3Pin, dir3Pin);
AccelStepper pump4 = AccelStepper(mit, step4Pin, dir4Pin);
/* AccelStepper pump5 = AccelStepper(mit, step5Pin, dir5Pin);
 * AccelStepper pump6 = AccelStepper(mit, step6Pin, dir6Pin);
 */
// user FSM states
enum use_states{
  NONE,
  READY,
  SELECTING,
  DISPENSING,
  DONE
};
use_states prior_state, state;

// mode states
enum modes{
  OFF,
  USER,
  SETUP
};
modes prior_mode, mode;

// setup FSM states
enum set_states{
  NOTSET,
  DISABLED,
  CLEAN,
  SET
};
set_states prev_state, curr_state;

// MAINTENANCE  FUNCTIONS 

void disabled(){
  // Menu mode for maintenance 
  uint32_t t;
  // Initialization mode
  if (curr_state != prev_state){
    prev_state = curr_state;
    digitalWrite(ena1, HIGH);
    digitalWrite(ena2, HIGH);
    lcd.clear();
    digitalWrite(WHITE, HIGH);
    dis_time = millis();
    dis_count = 0;
    lcd.setCursor(0,0);
    lcd.print("- BARBOT -");
    lcd.setCursor(0,1);
    lcd.print("- MAINTENANCE -");
    lcd.setCursor(0,2);
    lcd.print("- MODE -");
    delay(1000);
    lcd.setCursor(0,0);
    lcd.print("Set-Up    Clean");
    lcd.setCursor(0,1);
    lcd.print("VV           VV");
  }

  // Blink LEDs representing choice 
  t = millis();
  if(t >= dis_time + BLINK_INT){
      digitalWrite(RED, !digitalRead(RED));
      dis_time = t;
      dis_count++;
  }
  
  // Allow switch to maintenance options
  if (digitalRead(CONFIRM) == HIGH){
    while(digitalRead(CONFIRM) == HIGH) {}
    pump1.moveTo(1000000);
    pump2.moveTo(1000000);
    pump3.moveTo(1000000);
    pump4.moveTo(1000000);
    curr_state = CLEAN;
  } 
  if(digitalRead(SELECT) == HIGH){
    while(digitalRead(SELECT) == HIGH){}
    pump1.moveTo(1000000);
    pump2.moveTo(1000000);
    pump3.moveTo(1000000);
    pump4.moveTo(1000000);
    curr_state = SET;
  }
  if(digitalRead(RESET) == HIGH){
    while(digitalRead(RESET) == HIGH) {}
    mode = USER;
    pump1.setCurrentPosition(0);
    pump2.setCurrentPosition(0);
    pump3.setCurrentPosition(0);
    pump4.setCurrentPosition(0);
    lcd.clear();
    lcd.setCursor(4,0);
    lcd.print("Ready...");
    digitalWrite(WHITE, LOW);
  }
}

void clean(){
  // Pumpline Cleaning Mode
  uint32_t t;

  // Initialize 
  if (curr_state != prev_state){
    prev_state = curr_state;
    digitalWrite(ena1, LOW);
    digitalWrite(ena2, LOW);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Cleaning...");
    lcd.setCursor(0,1);
    lcd.print("RESET to Stop");
    clean_time = millis();
  }

  t = millis();
  if(t < cleandur + clean_time){
    digitalWrite(BLUE, HIGH);
    digitalWrite(WHITE, HIGH);
  }
  if(t > cleandur + clean_time - decel_time){
    pump1.stop();
    pump2.stop();
    pump3.stop();
    pump4.stop();
  }

  pump1.run();
  pump2.run();
  pump3.run();
  pump4.run(); 

  if(digitalRead(RESET) == HIGH){
    while(digitalRead(RESET) == HIGH){}
    pump1.stop();
    pump2.stop();
    pump3.stop();
    pump4.stop();
    pump1.setCurrentPosition(0);
    pump2.setCurrentPosition(0);
    pump3.setCurrentPosition(0);
    pump4.setCurrentPosition(0);
    digitalWrite(BLUE, LOW);
    digitalWrite(WHITE, LOW);
    curr_state = DISABLED;
  }
}

void set(){
  // Liquid Setting Mode
  uint32_t t;
  // Initialize
  if (curr_state != prev_state){
    prev_state = curr_state;
    digitalWrite(ena1, LOW);
    digitalWrite(ena2, LOW);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Setting Liquids");
    lcd.setCursor(0,1);
    lcd.print("Please Wait...");
    setup_time = millis();
  }

  t = millis();
  if(t < setupdur + setup_time){
    digitalWrite(BLUE, HIGH);
    digitalWrite(WHITE, HIGH);
  }
  if(t > setupdur + setup_time - decel_time){
    pump1.stop();
    pump2.stop();
    pump3.stop();
    pump4.stop();
  }

  pump1.run();
  pump2.run();
  pump3.run();
  pump4.run();

  // Stop motors and switch stage after set amount of time.
  t = millis();
  if (t > setupdur + setup_time){
    curr_state = DISABLED;
    pump1.setCurrentPosition(0);
    pump2.setCurrentPosition(0);
    pump3.setCurrentPosition(0);
    pump4.setCurrentPosition(0);
  }
  
  // delete later
  if(digitalRead(RESET) == HIGH){
    while(digitalRead(RESET) == HIGH){}
    pump1.stop();
    pump2.stop();
    pump3.stop();
    pump4.stop();
    pump1.setCurrentPosition(0);
    pump2.setCurrentPosition(0);
    pump3.setCurrentPosition(0);
    pump4.setCurrentPosition(0);
    digitalWrite(BLUE, LOW);
    digitalWrite(WHITE, LOW);
    curr_state = DISABLED;
  }
}


void idle(){
  // Wait for the SELECT button press, in the meantime, hold RED on.
  if (state != prior_state){
    prior_state = state;
    digitalWrite(RED, HIGH);
    digitalWrite(BLUE, HIGH);
    lcd.setCursor(4,0);
    lcd.print("Ready...");
    digitalWrite(ena1, HIGH);
    digitalWrite(ena2, HIGH);
    temp_choice_raw = analogRead(POTSELECT);
    temp_choice = temp_choice_raw/((1028/num_of_drinks) + 1);
    lcd.setCursor(0,1);
    lcd.print(drink_list[temp_choice]);
  }
  
  new_temp_choice_raw = analogRead(POTSELECT);
  new_temp_choice = new_temp_choice_raw/((1028/num_of_drinks) + 1);
  if(new_temp_choice != temp_choice){
    lcd.clear();
    lcd.setCursor(4,0);
    lcd.print("Ready...");
    temp_choice = new_temp_choice;
    lcd.setCursor(0,1);
    lcd.print(drink_list[temp_choice]);
  }
  if(digitalRead(SELECT) == HIGH){
    while(digitalRead(SELECT) == HIGH){}
    state = SELECTING;
    lcd.clear();
    digitalWrite(RED, LOW);
  }
  if(digitalRead(RESET) == HIGH){
    while(digitalRead(RESET) == HIGH){}
    mode = SETUP;
    prev_state = NOTSET;
    lcd.clear();
    digitalWrite(BLUE, LOW);
  }
}

void selecting(){
  // Read the analog once, and blink the LED mapping to the choice.
  // Give user 10 seconds to confirm selection.
  // if confirm -> DISPENSING, else -> READY.
  uint32_t t;
  // initialize selecting state
  if (state != prior_state){
    prior_state = state;
    choice_raw = analogRead(POTSELECT);
    drink_choice = choice_raw/((1028/num_of_drinks) + 1);
    lcd.setCursor(0,0);
    lcd.print("Current Drink:");
    lcd.setCursor(0,1);
    lcd.print(drink_list[drink_choice]);
    sel_time = millis();
    sel_count = 0;
    
  }

  // Blink LEDs representing choice
  t = millis();
  if(t >= sel_time + BLINK_INT){
    if(t >= sel_time + BLINK_INT){
      digitalWrite(RED, !digitalRead(RED));
      digitalWrite(YELLOW, !digitalRead(YELLOW));
      digitalWrite(GREEN, !digitalRead(GREEN));
    sel_time = t;
    sel_count++;
    }
  }

  // Check for state transition
  if (digitalRead(CONFIRM) == HIGH){
    while(digitalRead(CONFIRM) == HIGH) {}
    state = DISPENSING;
    pump1.moveTo(1000000);
    pump2.moveTo(1000000);
    pump3.moveTo(1000000);
    pump4.moveTo(1000000);
  } else if(sel_count == 20 || drink_choice == 0){
    state = READY;
  } else if(digitalRead(RESET) == HIGH){
    while(digitalRead(RESET) == HIGH){}
    state = READY;
  }
  
  //reset LEDs to switch out of selecting state
  if(state != prior_state){
    digitalWrite(RED, LOW);
    digitalWrite(YELLOW, LOW);
    digitalWrite(GREEN, LOW);
    lcd.clear();
  }
}

void dispensing(){
  // What to do while dispensing
  uint32_t t;
  
  // initialize dispensing state
  if(state != prior_state){
    prior_state = state;
    digitalWrite(YELLOW, HIGH);
    lcd.setCursor(0,0);
    lcd.print("Dispensing");
    lcd.setCursor(0,1);
    lcd.print(drink_list[drink_choice]);
    dispense_time = millis();
    digitalWrite(ena1, LOW);
    digitalWrite(ena2, LOW);
    for(int pd = 0; pd < num_of_motors; pd++){
      pump_durs[pd] = drink_array[drink_choice][pd];
    }
  }

  // REFACTORED CODE
  t = millis();
  if(t > (dispense_time + pump_durs[0] - decel_time)){
    pump1.stop();
  }
  if(t > (dispense_time + pump_durs[1] - decel_time)){
    pump2.stop();
  }
  if(t > (dispense_time + pump_durs[2] - decel_time)){
    pump3.stop();
  }
  if(t > (dispense_time + pump_durs[3] - decel_time)){
    pump4.stop();
  }
  if(t > dispense_time + pump_durs[0] && t > dispense_time + pump_durs[1] && t > dispense_time + pump_durs[2] && t > dispense_time + pump_durs[3]){
    state = DONE;
  }
  
  pump1.run();
  pump2.run();
  pump3.run();
  pump4.run();

  //if RESET button is hit, will abort action and return to READY
  if(digitalRead(RESET) == HIGH){
    while(digitalRead(RESET) == HIGH){}
    pump1.stop();
    pump2.stop();
    pump3.stop();
    pump4.stop();
    state = READY;
  }
  
  // Turns off motors to move on to the next state
  if(state != prior_state){
    digitalWrite(ena1, HIGH);
    digitalWrite(ena2, HIGH);
    digitalWrite(YELLOW, LOW);
    pump1.setCurrentPosition(0);
    pump2.setCurrentPosition(0);
    pump3.setCurrentPosition(0);
    pump4.setCurrentPosition(0);
    lcd.clear();
  }
}

void done(){
  // What to do when the barbot has finished.
  if(state != prior_state){
    prior_state = state;
    lcd.setCursor(5,0);
    lcd.print("Enjoy (:");
    digitalWrite(GREEN, HIGH);
  }
  delay(5000);
  digitalWrite(GREEN, LOW);
  lcd.clear();
  state = READY;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  // LCD/LED setup
  lcd.begin();
  lcd.backlight();
  pinMode(RED, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(WHITE, OUTPUT);
  digitalWrite(RED, HIGH);
  digitalWrite(YELLOW, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);
  digitalWrite(WHITE, LOW);

  // motor setup
  pinMode(ena1, OUTPUT);
  pinMode(ena2, OUTPUT);
  digitalWrite(ena1, HIGH);
  digitalWrite(ena2, HIGH);
  pump1.setMaxSpeed(max_speed);
  pump1.setAcceleration(acceleration);
  pump1.setSpeed(400);
  pump2.setMaxSpeed(max_speed);
  pump2.setAcceleration(acceleration);
  pump2.setSpeed(400);
  pump3.setMaxSpeed(max_speed);
  pump3.setAcceleration(acceleration);
  pump3.setSpeed(400);
  pump4.setMaxSpeed(max_speed);
  pump4.setAcceleration(acceleration);
  pump4.setSpeed(400);

  // Button Setup
  pinMode(SELECT, INPUT);
  pinMode(CONFIRM, INPUT);
  pinMode(RESET, INPUT);
  pinMode(POTSELECT, INPUT);

  //FSM setup
  prior_state = NONE;
  state = READY;
  prior_mode = OFF;
  mode = USER;
  prev_state = NOTSET;
  curr_state = DISABLED;
}

void loop() {
  // A state machine that loops through the process of dispensing a drink.
  // nested switch statements designed for user/setup modes
  switch(mode){
    case USER:
      switch(state){
        case READY:
        idle();
        break;
        case SELECTING:
        selecting();
        break;
        case DISPENSING:
        dispensing();
        break;
        case DONE:
        done();
        break;
      }
    break;
    case SETUP:
      switch(curr_state){
        case DISABLED:
        disabled();
        break;
        case CLEAN:
        clean();
        break;
        case SET:
        set();
        break;
      }
    break;
  }
}
