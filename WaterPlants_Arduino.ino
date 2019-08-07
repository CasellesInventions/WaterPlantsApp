// Water Plants App
//
// author: Eduardo Sesma Caselles
// email : e_sesma@hotmail.com
// date: 24 - June - 2018
//
// refs : https://www.rinconingenieril.es/appinventor-y-arduino-ethernet/
//        https://medium.com/@jvlobo/forget-about-watering-your-plants-2635efdf24b6



#include <Ethernet.h>

// #define DEBUG  //define for serial printing monitor

// ------------  PARAMETERS  -------------    //

#define WATERING_SECONDS  10

#define N_MOISTURE_SENSORS 5
#define TRANSISTOR_SECONDS 2   //used to warm up the current flux thru moisture sensors (avoid corrosion)
#define MAX_R_MOISTURE 1023
#define MIN_R_MOISTURE 250
#define MAX_WATER_DISTANCE_CM 100
#define MIN_WATER_DISTANCE_CM 73

#define AVG_READINGS 5

#define AUTO_CHECK_TIMER 60 //minutes
#define AUTO_AVG_TH_PERCENTAGE 40
#define COEFF_AVG_SENSOR_1 1
#define COEFF_AVG_SENSOR_2 1
#define COEFF_AVG_SENSOR_3 1
#define COEFF_AVG_SENSOR_4 1
#define COEFF_AVG_SENSOR_5 1

// ------------   SETUP  ----------------     //

//IDs:
#define RELAY_ID 2
#define TRANSISTOR_ID 6
#define ECHO_OUT_ID 4
#define ECHO_IN_ID 5
#define MOISTURE_1_ID A0
#define MOISTURE_2_ID A1
#define MOISTURE_3_ID A2
#define MOISTURE_4_ID A3
#define MOISTURE_5_ID A4
#define RESET_ID 7

int moisture_sensors_id[N_MOISTURE_SENSORS] = {MOISTURE_1_ID, MOISTURE_2_ID, MOISTURE_3_ID, MOISTURE_4_ID, MOISTURE_5_ID};
float coeff_avg_sensors[N_MOISTURE_SENSORS] = {COEFF_AVG_SENSOR_1, COEFF_AVG_SENSOR_2, COEFF_AVG_SENSOR_3, COEFF_AVG_SENSOR_4, COEFF_AVG_SENSOR_5};

//Internal MAC/IP configuration:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // GENERAL MAC
IPAddress ip(192, 168, 0, 000); // WRITE HERE YOUR LOCAL IP FOR THE SERVER

//Port HTTP:
EthernetServer server(81);

// ------------  Variables --------------     //

String readString;
int auto_mode = 0;
int moisture_levels[N_MOISTURE_SENSORS];

//Timer:
long last_time = 0;
int minutes = 0;
int hours = 0;
int days = -100;  // init like that to know if arduino was reset
long last_time_auto = days*24*60; // to start counting with the same amount of days as timer in mins.


void setup()
{
  //Init serial and network conection
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.begin(9600);
#ifdef DEBUG
  Serial.println("Initializing...");
  Serial.print("AUTO = ");
  Serial.println(auto_mode);
#endif
  pinMode(TRANSISTOR_ID, OUTPUT);
  digitalWrite(TRANSISTOR_ID, LOW);
  pinMode(RELAY_ID, OUTPUT);
  digitalWrite(RELAY_ID, LOW);
  pinMode(ECHO_OUT_ID, OUTPUT);
  pinMode(ECHO_IN_ID, INPUT);
  pinMode(RESET_ID, OUTPUT);
  digitalWrite(RESET_ID, HIGH);
}


// ------------------   MAIN   ---------------------    //
void loop()
{
  timer_counter();                                             // check timer for overflow

  if (auto_mode)
  {
    int elapsed_time = get_auto_elapsed_time_mins();
    if (elapsed_time > AUTO_CHECK_TIMER)
    {
      last_time_auto = last_time_auto + elapsed_time;
      open_moisture_readings(TRANSISTOR_SECONDS);
      read_all_moisture_levels();
      close_moisture_readings();
      if (avg_moisture_levels() < AUTO_AVG_TH_PERCENTAGE)
      {
        water(WATERING_SECONDS);
        last_time_auto = 0;
      }
    }
  }

  EthernetClient client = server.available();                 // wait for client
  if (client)
  {
    while (client.connected())                                // client connected
    {
      if (client.available())
      {
        char client_char = client.read();
        if (readString.length() < 100)                        //acumulate msg in the string
        {
          readString += client_char;
        }
        if (client_char == 'n')                               //client call is finished
        {
          client.println("HTTP/1.1 200 OK");                  //Sending web header
          client.println("Content-Type: text/html");
          client.println();

          if (readString.indexOf("?WATERa") > 0)
          {
            client.println("<center> <h2> Watering!! </h2>  ");  //Show web
            msg_ending(client);

            water(WATERING_SECONDS);

            client.println("<center> <h2> Stop Watering!! </h2> ");  //Show web
            msg_ending(client);
          }

          else if (readString.indexOf("?GET_MOISTUREa") > 0)
          {
            client.println("<center> <h2> Getting moisture levels... </h2> "); //Show web
            msg_ending(client);
#ifdef DEBUG
            Serial.println("Getting moisture levels...");
#endif
            open_moisture_readings(TRANSISTOR_SECONDS);
            read_all_moisture_levels();
            close_moisture_readings();
            show_web_moisture(client, moisture_levels);
          }

          else if (readString.indexOf("?GET_TIMERa") > 0)
          {
#ifdef DEBUG
            Serial.println("Getting last time...");
            Serial.print(days);
            Serial.print("days : ");
            Serial.print(hours);
            Serial.print("hours : ");
            Serial.print(minutes);
            Serial.println("minutes.");
#endif
            show_web_timer(client);
          }

          else if (readString.indexOf("?GET_AUTO_LASTa") > 0)
          {
#ifdef DEBUG
            Serial.println("Getting last time auto...");
#endif
            int elapsed_mins = get_auto_elapsed_time_mins();
#ifdef DEBUG
            Serial.print(elapsed_mins);
#endif
            show_web_timer_auto(client, elapsed_mins);
          }

          else if (readString.indexOf("?AUTO_ONa") > 0)
          {
            auto_mode = 1;
            client.println("<center> <h2>AUTO mode ON <h2>"); //Show web
#ifdef DEBUG
            Serial.print("AUTO = ");
            Serial.println(auto_mode);
#endif
            msg_ending(client);
          }

          else if (readString.indexOf("?AUTO_OFFa") > 0)
          {
            auto_mode = 0;
            client.println("<center> <h2>AUTO mode OFF <h2>"); //Show web
#ifdef DEBUG
            Serial.print("AUTO = ");
            Serial.println(auto_mode);
#endif
            msg_ending(client);
          }

          else if (readString.indexOf("?WATER_LEVELa") > 0)
          {
            int water_level = get_water_level();
            show_web_water_level(client, water_level);
          }

          else if (readString.indexOf("?RESETa") > 0)
          {
#ifdef DEBUG
            Serial.println("Reset");
#endif
            client.println("<center> <h2> Reseting... </h2> "); //Show web
            msg_ending(client);
            show_web_menu(client);
            delay(200);
            digitalWrite(RESET_ID, LOW);
            delay(2000);
#ifdef DEBUG
            Serial.println("Reset failed");
#endif
            client.println("<center> <h2> Reset FAILED! </h2> "); //Show web
            msg_ending(client);
          }

          show_web_menu(client);
          break;
        } //message
      } //client available
    } // client connected

    delay(1);             // wait for next data
    readString = "";      // Flush string for next reading
    client.stop();       // close connection
  }
}


// ----------------- FUNCTIONS ----------------- //

// WATER
void water(int t)
{
#ifdef DEBUG
  Serial.println("water ON");
#endif
  digitalWrite(RELAY_ID, HIGH);
  delay(t * 1000);
  digitalWrite(RELAY_ID, LOW);
#ifdef DEBUG
  Serial.println("water OFF");
#endif
  init_timer();
}



//MOISTURE
void open_moisture_readings(int transistor_waiting_time)
{
  digitalWrite(TRANSISTOR_ID, HIGH);
  delay(transistor_waiting_time * 1000);
}

void close_moisture_readings()
{
  digitalWrite(TRANSISTOR_ID, LOW);
}

int avg_moisture_levels()
{
  int avg = 0;
  for (int i_sensor = 0; i_sensor < N_MOISTURE_SENSORS; i_sensor++)
  {
    avg = avg + coeff_avg_sensors[i_sensor] * moisture_levels[i_sensor];
  }
  avg = avg / N_MOISTURE_SENSORS;
#ifdef DEBUG
  Serial.print("AUTO Check: avg moisture level: ");
  Serial.println(avg);
#endif

  return avg;
}

void read_all_moisture_levels()
{
  // do first reading for each sensor
  for (int i_sensor = 0; i_sensor < N_MOISTURE_SENSORS; i_sensor++)
  {
    moisture_levels[i_sensor] = analogRead(moisture_sensors_id[i_sensor]);
  }
  delay(1000);
  //accumulate de readings sequentially, so we can wait between readings of the same sensor.
  for (int i_avg = 0; i_avg < (AVG_READINGS - 1); i_avg++)
  {
    for (int i_sensor = 0; i_sensor < N_MOISTURE_SENSORS; i_sensor++)
    {
      moisture_levels[i_sensor] = moisture_levels[i_sensor] + analogRead(moisture_sensors_id[i_sensor]);
    }
    delay(1000);
  }
  // divide by the number of readings and map to percentage
  for (int i_sensor = 0; i_sensor < N_MOISTURE_SENSORS; i_sensor++)
  {
    moisture_levels[i_sensor] = map( (moisture_levels[i_sensor] / AVG_READINGS) , MAX_R_MOISTURE, MIN_R_MOISTURE, 0, 100) ;
#ifdef DEBUG
    Serial.print("moisture level ");
    Serial.print(i_sensor + 1);
    Serial.print(" : ");
    Serial.println(moisture_levels[i_sensor]);
#endif
  }
}

//TIMER
void timer_counter()
{
  if ( (millis() - last_time) > 60000)
  {
    minutes++;
    last_time = millis();
  }
  if (minutes > 60)
  {
    hours++;
    minutes = 0;
  }
  if (hours > 24)
  {
    days++;
    hours = 0;
  }

}

void init_timer()
{
  if(auto_mode)
  {
    last_time_auto = - get_auto_elapsed_time_mins(); //because we will init timer at 0: last_time_auto = 0 - elapsed_time
  }
  days = 0;
  hours = 0;
  minutes = 0;
}

int get_auto_elapsed_time_mins()
{
  return (int) ((days * 24 * 60 + hours * 60 + minutes) - last_time_auto); // timer_mins -  last_time_auto_mins
}

//WATER LEVEL
int get_water_level()
{
  unsigned long avg_distance = 0;
  for (int i_sensor = 0; i_sensor < AVG_READINGS; i_sensor++)
  {
    // Shoot ultrasound
    digitalWrite(ECHO_OUT_ID, LOW);
    delayMicroseconds(2);
    digitalWrite(ECHO_OUT_ID, HIGH);
    delayMicroseconds(10);
    digitalWrite(ECHO_OUT_ID, LOW);

    unsigned long t = pulseIn(ECHO_IN_ID, HIGH);

    avg_distance = avg_distance + (t * 0.000001 * 34000.0 / 2.0);  // cm = t * microsecs * sound speed(cm/s) / 2 ways (go and return)
    delay(500);
  }
  int distance = ((int)avg_distance) / AVG_READINGS;
  int water_level = map(distance, MAX_WATER_DISTANCE_CM, MIN_WATER_DISTANCE_CM, 0, 100);

#ifdef DEBUG
  Serial.print(distance);
  Serial.println(" cm - ");
  Serial.print(water_level);
  Serial.println(" % water level");
#endif

  return water_level;
}

//HTTP
void show_web_menu(EthernetClient client)
{
  client.println("<center> <h2> Watering System </h2> ");
  client.println("<a href='./?WATERa'>WATER 10 seconds</a>");
  client.println("<br>");
  client.println("<a href='./?GET_MOISTUREa'>GET moisture levels</a> ");
  client.println("<br>");
  client.println("<a href='./?GET_TIMERa'>Get time last time watering</a>");
  client.println("<br>");
  client.print("AUTO: <a href='./?AUTO_ONa'>ENABLE</a> - <a href='./?AUTO_OFFa'>DISABLE</a>. -- ");
  client.println("<a href='./?GET_AUTO_LASTa'>Get minutes since checking</a>");
  client.println("<br>");
  client.println("<a href='./?WATER_LEVELa'>Get water level </a>");
  client.println("<br>");
  client.println("<br> <a href='./?RESETa'>RESET</a>");
  msg_ending(client);
}

void show_web_moisture(EthernetClient client, int moisture_levels[N_MOISTURE_SENSORS])
{
  for (int i_sensor = 0; i_sensor < N_MOISTURE_SENSORS; i_sensor++)
  {
    client.print("<center> <h2> Moisture level ");
    client.print(i_sensor + 1);
    client.print(" : ");
    client.print(moisture_levels[i_sensor]);
    client.println(" % </h2>");
  }
  msg_ending(client);
}

void show_web_timer(EthernetClient client)
{
  client.print("<center> <h2> Last time watered: ");
  client.print(days);
  client.print(" D: ");
  client.print(hours);
  client.print(" H: ");
  client.print(minutes);
  client.print(" M      </h2> ");
  msg_ending(client);
}

void show_web_timer_auto(EthernetClient client, int mins)
{
  client.print("<center> <h2> Last time checked auto: ");
  client.print(mins);
  client.print(" Minutes    </h2> ");
  msg_ending(client);
}

void show_web_water_level(EthernetClient client, int water_level)
{
  client.print("<center> <h2> Water level: ");
  client.print(water_level);
  client.print(" %    </h2> ");
  msg_ending(client);
}

void msg_ending(EthernetClient client)
{
  client.println(" </center>");
  client.println("<br />");
}

