#include <SoftPWM.h>
#include <Color.h>

#define LED_COUNT 12
#define LED_START_PIN 8
//byte led_pins[LED_COUNT * 3];
//#define ULTRASONIC1_TRIGGER_PIN 2
//#define ULTRASONIC1_ECHO_PIN 3
//#define ULTRASONIC2_TRIGGER_PIN 4
//#define ULTRASONIC2_ECHO_PIN 5

#define ULTRASONIC_GND_PIN A12
#define ULTRASONIC_VCC_PIN A15
#define ULTRASONIC_TRIGGER_PIN A14 
#define ULTRASONIC_ECHO_PIN A13

#define ALPHA 0.8
#define TRIGGER_THR 3
#define FSM_CLOSED 0
#define FSM_IDLE 1
#define FSM_TRANSITION_UP 2
#define FSM_TRANSITION_DOWN 3
#define FSM_INTRUSION 4

#define DIST_2_OFFSET_RATIO 0.09


Color cur_color = Color(1,1,1);
float sat = 1.0;
float val = 0.5;
float hue = 0;
float offset = 0;
float f_dist = 0.0;
float f_dist_old = 0.0;
float d_dist = 0.0;
int state = FSM_IDLE;



int ultrasonic_pins[2][2] = {
  {ULTRASONIC_TRIGGER_PIN,ULTRASONIC_ECHO_PIN},
  {ULTRASONIC_TRIGGER_PIN,ULTRASONIC_ECHO_PIN}
};

void setup() {
  Serial.begin(9600);
  for(int i = 0; i < LED_COUNT * 3; ++i) {
    pinMode(LED_START_PIN+i, OUTPUT); 
    digitalWrite(LED_START_PIN+i,LOW);
    delay(200);
    digitalWrite(LED_START_PIN+i,HIGH);
    
  }
  
  delay(3000);
  
  pinMode(ULTRASONIC_GND_PIN, OUTPUT);
  digitalWrite(ULTRASONIC_GND_PIN, LOW);
  pinMode(ULTRASONIC_VCC_PIN, OUTPUT);
  digitalWrite(ULTRASONIC_VCC_PIN, HIGH);
  
  SoftPWMBegin();
}

int counter_up = 0;
int counter_down = 0;
int counter_idle = 0;


void loop() {
  int distance = ultrasonic_distance(0);
  if (state == FSM_IDLE or state == FSM_CLOSED) {
    if (state == FSM_IDLE) {
      counter_idle += 1;
    }
    if (counter_idle > 30) {
      state = FSM_CLOSED;
      counter_idle = 0; 
    } else if (distance >= TRIGGER_THR) {
      state = FSM_TRANSITION_UP;
      counter_up = 1;
      d_dist = 0.0; // f_dist - f_dist_old;
      f_dist = distance;
    } else {
      d_dist = 0.0;
    }
  }
  if (state == FSM_TRANSITION_UP) {
    if (distance >= TRIGGER_THR) {
      counter_up += 1;
    } else {
      counter_up -= 1;
    }
    if (counter_up > 3) {
      state = FSM_INTRUSION;
      f_dist = distance;
    }
    if (counter_up <0) {
      state = FSM_CLOSED;
      }
    d_dist = 0.0;
  }
  if (state == FSM_TRANSITION_DOWN) {
    if (distance < TRIGGER_THR) {
      counter_down += 1;
    } else {
      counter_down -= 1;
    }
    if (counter_down > 3) {
      state = FSM_IDLE;
    }
    if (counter_down < 0) {
      state = FSM_INTRUSION;
      counter_idle = 0;
      f_dist = distance;
    }
    d_dist = 0.0;
  }
  if (state == FSM_INTRUSION) {
    f_dist_old = f_dist;
    f_dist = f_dist_old * ALPHA + distance * ( 1 - ALPHA );
    if (distance < TRIGGER_THR) {
      state = FSM_TRANSITION_DOWN;
      counter_down = 0;
      d_dist = 0.0;
    } else {
      d_dist = f_dist - f_dist_old;
    }
  }
  
  offset = offset + d_dist * DIST_2_OFFSET_RATIO;
  if ( offset >=1 ) offset = 0; 
  if ( offset <0 ) offset = 1;
  
  if (state == FSM_CLOSED) {
    shut_all_leds();
  } else {
    for(int i = 0; i < LED_COUNT; ++i) {
      hue = offset + (float(i)/LED_COUNT);
      if ( hue >=1 ) hue = 0;
      cur_color.convert_hcl_to_rgb(hue,sat,val);
      set_led_rgb(i, cur_color);
    }
  }
  //offset += 1.0/LED_COUNT/10;

  
  Serial.print(distance);
  Serial.print("\tstate: ");
  Serial.print(state);
  Serial.print("\td_dist: ");
  Serial.print(d_dist);
  Serial.print("\tf_dist: ");
  Serial.print(f_dist);
  Serial.print("\tf_dist_old: ");
  Serial.print(f_dist_old);
  Serial.print("\tcounter_idle: ");
  Serial.print(counter_idle);
  Serial.print("\toffset: ");
  Serial.println(offset);
  delay(20);
}

void shut_all_leds() {
  for(int i = 0; i < 3 * LED_COUNT; ++i) {
    SoftPWMSet(i, 255);
  }
}

void set_led_rgb(int led_idx, Color c) {
  SoftPWMSet(LED_START_PIN + 3 * led_idx + 0, c.red);
  SoftPWMSet(LED_START_PIN + 3 * led_idx + 1, c.green);
  SoftPWMSet(LED_START_PIN + 3 * led_idx + 2, c.blue);
}

int ultrasonic_distance(int idx) {
  int trigPin = ultrasonic_pins[idx][0];
  int echoPin = ultrasonic_pins[idx][1];
  long duration;
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(20);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH, 6000);
  duration = duration / 59;
  if ((duration < 2) || (duration > 30)) return false;
  return duration;
}
