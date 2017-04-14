#include <SimpleTimer.h>
#include <Wire.h>
#include <string.h>

#define PostLaunch 0
#define Beaconing 1
#define Recharge 2
#define Failsafe 3
#define Telecom 4

#define START_MSG "start"
#define SLEEP_MSG "sleep"
#define STOP_MSG "stop"
#define TEST_MSG "test"

#define STACK_SIZE 3

// check maximum time for the timer
SimpleTimer timer;

// Return status code for functions to enable exception Handling
class Status
{
  int returnCode;
  Status (int returnCode)
  {
    this.returnCode = returnCode;
  }

  bool success ()
  {
    return (this.returnCode == 0);
  }

  bool fail ()
  {
    return (this.returnCode != 0);
  }

}


// =======================================
// ADCS Function
// convert into class
// =======================================
class ADCS ()
{
  // Define ADCS data structure
  int x, y, z;

  ACDS(int x, int y, int z)
  {
    this.x = x;
    this.y = y;
    this.z = z;
  }

  Status transmitACDS (string acdsAddress)
  {
      Wire.beginTransmission (acdsAddress);
      Wire.write(START_MSG);
      Serial.println("Starting ADCS Transmission");
      Wire.endTransmission();
      delay(500);
      Wire.beginTransmission(acdsAddress);

      Wire.write(5); // assume x
      Wire.write(6); // assume y
      Wire.write(7); // assume zp

      Wire.endTransmission();

      Wire.requestFrom(acdsAddress, 3);

      if (Wire.available() <= 3)
      {
        x = Wire.read();
        y = Wire.read();
        z = Wire.read();
      } else {
        return Status(1);
      }

      Serial.print("X =");
      Serial.print(x);
      Serial.print("Y =");
      Serial.print(y);
      Serial.print("Z =");
      Serial.print(z);

      return Status(0);
  }
}

// =======================================
// Communication Class
// Unification of I2C and Serial communication
// =======================================
class Communication
{
    bool I2C;
    bool serial;

    // ========================================
    // Default Communication Class constructor
    // ========================================
    Communication (bool I2C, bool serial)
    {
        this.I2C = I2C;
        this.serial = serial;
    }

    Status startCommunication(string commAddress)
    {
      return Status(0);
    }

    Status stopCommunication()
    {
      return Status(0);
    }
};

// =======================================
// TimeStamp Structure
// From the on board clock on the OBC
// =======================================
class TimeStamp
{
  // These variables use the Short data type
  // See: https://www.arduino.cc/en/Reference/Short
  short year;
  short month;
  short day;
  short hour;
  short minute;
  short seconds;

  TimeStamp(short year, short month, short day, short minute, short seconds)
  {
    this.year = year;
    this.month = month;
    this.day = day;
    this.minute = minute;
    this.seconds = seconds;
  }
};

// =======================================
// Power Data Structure
// Handling data from EPS and from on board power on OBC
// =======================================
class PowerData
{
  float voltage;
  float current;

  // ================================================
  // Default Constructor for the Power Data Structure
  // ================================================
  PowerData(float voltage, float current)
  {
    this.voltage = voltage;
    this.current = current;
  }
};

// =======================================
// Beacon Data Structure
// Handling data that will be communicated through morse code
// =======================================
class BeaconData
{
  string callSign;
  string message;
  int messageLength;

  BeaconData (string callSign,
  string message,
  int messageLength)
  {
    this.callSign = callSign;
    this.message = message;
    this.messageLength = messageLength;
  }
};

// =======================================
// Telemetry Data Structure
// Data about the cubesat to be stored on the OBC
// and transmitted to ground station
// =======================================
class TelemetryData
{
  TimeStamp TS;
  PowerData PD;

  short id; // counter, sequence number of the telemetry data entry
  bool payloadAlive;
  float systemTemperature;

  // ====================================================
  // Default Constructor for the Telemetry Data Structure
  // ====================================================
  TelemetryData(short id, bool payloadAlive, float systemTemperature)
  {
    this.id = id;
    this.payloadAlive = payloadAlive;
    this.systemTemperature = systemTemperature;
  }

  TelemetryData(TelemetryData copyConstructor)
  {
    this.id = copyConstructor.id;
    this.payloadAlive = copyConstructor.payloadAlive;
    this.systemTemperature = copyConstructor.systemTemperature;

    this.TS = copyConstructor.getTimeStamp();
    this.PD = copyConstructor.getPowerData();
  }

  void setTimeStamp(TimeStamp TS)
  {
    this.TS = TS;
  }

  TimeStamp getTimeStamp()
  {
    return this.TS;
  }

  void setPowerData(PowerData PD)
  {
    this.PD = PD;
  }

  PowerData getPowerData()
  {
    return this.PD;
  }
};

// =======================================
// Payload Data Structure
// Data structure to rx and tx with the payload
// =======================================
class PayloadData
{
  bool TxOrRx;
  string TXmessage;

  bool TXready;
  bool RXready;

  // ====================================================
  // Default Constructor for the Telemetry Data Structure
  // ====================================================
  PayloadData (bool TxOrRx, string TXmessage)
  {
    this.TxOrRx = TxOrRx;
    this.TXmessage = TXmessage;
  }

  void setTXready(bool TXready)
  {
    this.TXready = TXready;
  }

  void setRXready(bool RXready)
  {
    this.RXready = RXready;
  }
};



// =======================================
// On Board Computer Data Class
// Data structure for the OBC
// =======================================
class OBCData
{
  bool antennaDeployed;
  bool postLaunchComplete;
  int currentState;
  bool telemetryRequests;

  // ====================================================
  // Default Constructor for the Telemetry Data Structure
  // ====================================================
  OBCData (bool antennaDeployed,
    bool postLaunchComplete,
    int currentState,
    bool telemetryRequests)
  {
    this.antennaDeployed = antennaDeployed;
    this.postLaunchComplete = postLaunchComplete;
    this.currentState = currentState;
    this.telemetryRequests = telemetryRequests;
  }
};


// =======================================
// General Data Class
// =======================================
class Data
{
  public:
    TelemetryData telData[STACK_SIZE];
    int telCount;

    PayloadData payData[STACK_SIZE];
    int payCount;

    OBCData obcData[STACK_SIZE];
    int obcCount;

    // ====================================================
    // Default Constructor for the General Data class
    // ====================================================
    Data ()
    {
      telCount = -1;
      payCount = -1;
      obcCount = -1;
    }

    // ======================================================
    // Add Telemetry Entry function inside General Data class
    // ======================================================
    Status addTelemetryEntry (TelemetryData t)
    {
      if ((telCount + 1) < STACK_SIZE)
      {
        telCount++;
        telData[telCount] = t;
      }
      else
      {
        for (int i = 1; i < STACK_SIZE; i++)
        {
          telData[i - 1] = telData[i];
        }
        telData[telCount] = t;
      }

      return Status(0);
    }

    // =====================================================
    // General Data Class: Adding Payload Entry function
    // =====================================================
    Status addPayloadEntry (PayloadData p)
    {
      if ((payCount + 1) < STACK_SIZE)
      {
        payCount++;
        payData[payCount] = p;
      }
      else
      {
        for (int i = 1; i < STACK_SIZE; i++)
        {
          payData[i - 1] = payData[i];
        }
        payData[payCount] = p;
      }

      return Status(0);
    }

    // =====================================================
    // General Data Class: Adding OBC Entry function
    // =====================================================
    Status addOBCEntry (OBCData o)
    {
      if ((obcCount + 1) < STACK_SIZE)
      {
        obcCount++;
        obcData[obcCount] = o;
      }
      else
      {
        for (int i = 1; i < STACK_SIZE; i++)
        {
          obcData[i - 1] = obcData[i];
        }
        obcData[obcCount] = o;
      }

      return Status(0);
    }
    // =====================================================
      // General Data Class: Deleting a Telemetry Row function
      // =====================================================
 -    Status deleteTelemetryRow()
 +    Status deleteِِTelemetryRow(int numberOfRowsToDelete, int index[STACK_SIZE]) 
      {
 +      for (int i=0; i<numberOfRowsToDelete; i++)
 +      {
 +        for (int j=index[i]; j<STACK_SIZE; j++)
 +        {
 +          telData[j]=telData[j+1];
 +        }
 +        telCount--;
          if (index[i]<index[i+1]) index[i+1]--;
 +      }
          return Status(0);
      }
 +    // =====================================================
 +    // General Data Class: Deleting all Telemetry Data function
 +    // =====================================================
 +    Status deleteAllTelemetryData()
 +    {
 +      telCount=0; // this will simply take me to the start of the stack rendering the other data inaccessable
 +      // i don't know if there is any function for freeing memory in arduino
 +    }
  
      // ====================================================
      // General Data Class: Deleting a Payload Row function
      // ====================================================
 -    Status deletePayloadRow()
 +    Status deletePayloadRow(int numberOfRowsToDelete, int index[STACK_SIZE])
      {
 +        for (int i=0; i<numberOfRowsToDelete; i++)
 +      {
 +        for (int j=index[i]; j<STACK_SIZE; j++)
 +        {
 +          payData[j]=payData[j+1];
 +        }
 +        payCount--;
          if (index[i]<index[i+1]) index[i+1]--;
 +      }
          return Status(0);
      }
 +  
 +    // =====================================================
 +    // General Data Class: Deleting all Payload Data function
 +    // =====================================================
 +    Status deleteAllPayData()
 +    {
 +      payCount=0; // this will simply take me to the start of the stack rendering the other data inaccessable
 +      // i don't know if there is any function for freeing memory in arduino
 +    }
  
      // ====================================================
      // General Data Class: Deleting a OBC Row function
      // ====================================================
 -    Status deleteOBCRow()
 +    Status deleteOBCRow(int numberOfRowsToDelete, int index[STACK_SIZE]) // we can improve her by sending the time stap instead of 
                                                                           // the index but i don't know the frame setup yet
      {
 +        for (int i=0; i<numberOfRowsToDelete; i++)
 +      {
 +        for (int j=index[i]; j<STACK_SIZE; j++)
 +        {
 +          obcData[j]=obcData[j+1];
 +        }
 +        obcCount--;
          if (index[i]<index[i+1]) index[i+1]--;
 +      }
          return Status(0);
      }
 +    // =====================================================
 +    // General Data Class: Deleting all OBD Data function
 +    // =====================================================
 +    Status deleteAllOBCData()
 +    {
 +      obcCount=0; // this will simply take me to the start of the stack rendering the other data inaccessable
 +      // i don't know if there is any function for freeing memory in arduino
 +    }
  };
// ====================================================
// Cubesat States Class
// ====================================================
class States
{
  public:
    int currentState;

    // ====================================================
    // Changing the Current Cubesat State function
    // ====================================================
    void changeCurrentState(Data data)
    {
      if (data.postLaunchComplete)
      {
        if (data.batteryLevel > 0.6 && !data.telemetryRequests)
        {
          currentState = Beaconing;
        }
        else if (data.batteryLevel > 0.9 &&  data.telemetryRequests)
        {
          currentState = Telecom;
        }
        else if (data.batteryLevel < 0.6 && data.batteryLevel > 0.2)
        {
          currentState = Recharge;
        }
        else if (data.batteryLevel < 0.2)
        {
          currentState = Failsafe;
        }
      }
      else
      {
        currentState = PostLaunch;
      }
    }

    // ====================================================
    // Default States class constructor
    // ====================================================
    States()
    {
      currentState = 0;
    }
};

// ====================================================
// General Cubesat Class
// ====================================================
class Cubesat
{
  public:
    States JY1STATES;
    Data JY1DATA;
    Communication com;

    // ====================================================
    // Default Constructor for the General Cubesat Class
    // ====================================================
    Cubesat()
    {
      // check if current state is post launch
      JY1DATA.antennaDeployed = true;
      JY1DATA.postLaunchComplete = true;
      JY1DATA.currentState = PostLaunch;
      JY1DATA.telemetryRequests = false;
      JY1DATA.batteryLevel = 0.9;
    }

    // ====================================================
    // Entering Orbital Mode Delay function
    // 1. Send sleep transmission to all controllers
    // 2. Wait 15 minutes
    // ====================================================
    void orbitalModeDelay ()
    {
      Wire.beginTransmission(98); // transmit to device's
      Wire.write(SLEEP_MSG);        // sends five bytes
      Wire.endTransmission();    // stop transmitting
      Serial.print("orbit mode");

      // timer does 15 mintues sleep

      return Status(0);
    }

    // ====================================================
    // Checking the Radio Frequency function
    // Check if payload is alive; return status of RF payload
    // ====================================================
    Status checkRadioFrequency ()
    {
      Wire.beginTransmission(8); // transmit to device #8
      Wire.write(START_MSG);        // sends five bytes
      Serial.println("Starting Transmission");
      Wire.endTransmission();    // stop transmitting

      delay(500);

      Wire.beginTransmission(8); // transmit to device #8
      Wire.write(TEST_MSG);
      Serial.println("JY1SAT TEST");
      Wire.endTransmission();    // stop transmitting

      delay(1000);

      Wire.beginTransmission(8); // transmit to device #8
      Wire.write(STOP_MSG);
      Serial.println("Stopping Transmission");
      Wire.endTransmission();    // stop transmitting

      delay(1000);

      return Status(0);
    }

    // ====================================================
    // Checking the Battery function
    // Return the EPS state as an EPS class data structure
    // ====================================================
    Status checkBattery ()
    {
      Wire.beginTransmission(78); // transmit to eps's
      Wire.write(START_MSG);        // sends five bytes
      Wire.endTransmission();     // stop transmitting
      Wire.beginTransmission(77);
      Wire.write(5);              // assume x
      Wire.endTransmission();
      Wire.requestFrom(80, 3);    // ASSUME EPS 80
      if (Wire.available() <= 1)
      {
        JY1DATA.batteryLevel = Wire.read();
      }
      else
      {
        return Status(1);
      }

      Serial.print("BATTERY LEVEL = ");
      Serial.print(JY1DATA.batteryLevel);

      return Status(0);
    }

    // ====================================================
    // Cubesat Post Launch function
    // ====================================================
    Status postLaunch() // post launch function does not throw any exceptions
    {
      timer.run();

      Status orbitalStatus = orbitalModeDelay(); // Sleep for 15 min for other subsystems
      if (orbitalStatus.fail())
      {
          // handle orbital mode delay failiure here
      }

      Status batteryStatus = checkBattery ();
      if (batteryStatus.fail())
      {
          // handle battery status check failiure here
      }

      Status rfStatus = checkRadioFrequency ();
      if (rfStatus.fail())
      {
          // handle RF check failiure here
      }

      return Status(0);
    }

    // ====================================================
    // Cubesat Fail Safe State function
    // ====================================================
    Status failSafe()
    {
      Wire.beginTransmission(8); // transmit to device #8
      Wire.write(SLEEP_MSG);        // sends five bytes
      Wire.endTransmission();    // stop transmitting

      return Status(0);
    }

    // ====================================================
    // Cubesat Recharging State function
    // ====================================================
    Status recharging()
    {
      return Status(0);
    }

    // ====================================================
    // Cubesat Beaconing State Function
    // ====================================================
    Status beaconing()
    {
      Serial.println("We are in beaconing mode");
      
 int temp_add=72,power_add=73,payload_add=74,telecom_add=75;
      
      
      Serial.println("We are in beaconing mode");
      Wire.beginTransmission(temp_add); // send request , start talking to get the temprature  
      Wire.send(0); // ask for data in register 0 
      Wire.endTransmission();    // stop transmitting
      Wire.requestFrom(temp_add, 1);    // request 1 bytes from temp add
      while (Wire.available()==0);
      int c = Wire.read() ; // the value of temprature in variable c 
      Wire.beginTransmission(power_add); // send request , start talking to get the current and voltage  
      Wire.send(0); // ask for data in register 0 , assume voltage 
      Wire.send(1); // // ask for data in register , assume current 
      Wire.endTransmission();    // stop transmitting
      Wire.requestFrom(power_add, 2);  // request 2 bytes from power add
      if (Wire.available() <= 2)
      {
        int c1 = Wire.read(); // the value of vlotage in variable c1
        int c2 = Wire.read();  // the value of current in variable c2
      }
      
      Wire.beginTransmission(payload_add); // send request , start talking to get the status of payload alive   
      Wire.send(0); // ask for data in register 0 
      Wire.endTransmission();    // stop transmitting
      Wire.requestFrom(payload_add, 1);    // request 1 bytes from temp add
      while (Wire.available()==0);
      bool c3 = Wire.read() ; // the value of temprature in variable c3
      string MSGG;
      string tmp1;
      int tmp2;
      if(c3)
      MSGG+="Payload Alive : True";
      else
      MSGG+="Payload Alive : False";
      tmp2=c;
      tmp1="";
      while(tmp2){
        tmp1+=(tmp2%10)+'0';
        tmp2/=10;
      }
      MSGG+=" "+"Temprature in celsius = "    +string(tmp1.rbegin(),tmp1.rend());
       tmp2 = c1;
      tmp1="";
      while(tmp2){
        tmp1+=(tmp2%10)+'0';
        tmp2/=10;
      }
      MSGG+=" "+"The voltage = "+string(tmp1.rbegin(),tmp1.rend());
     tmp2 = c2;
      tmp1="";
      while(tmp2){
        tmp1+=(tmp2%10)+'0';
        tmp2/=10;
      }
      MSGG+=" "+"The current = "+string(tmp1.rbegin(),tmp1.rend());

      Wire.beginTransmission(telecom_add); // transmit to telecommunications device
      Wire.write(START_MSG);        // sends Start MSH
      Wire.endTransmission();    // stop transmitting

      delay(500);

      Wire.beginTransmission(telecom_add); // transmit to  telecommunications device , Commanad to inform we have becon msg to send
      Wire.write("Send Beacon");
      Wire.endTransmission();    // stop transmitting
      delay (500);

       Wire.beginTransmission(telecom_add); // transmit to telecommunications device 
       Wire.write(MSGG); // send becon msg
       Wire.endTransmission();    // stop transmitting
      return Status(0);
    }

    // ====================================================
    // Cubesat Telecommunications Function
    // ====================================================
    void telecom()
    {
      return Status(0);
    }
};

// ====================================================
// Arduino: Global Variables Initializations
// ====================================================
Cubesat JY1SAT;
TelemetryData tt;

// ====================================================
// Arduino: Setup Function
// ====================================================
void setup()
{
  Wire.begin();
  Serial.begin(9600);
}

// ====================================================
// Arduino: Loop Function
// ====================================================
void loop()
{
  JY1SAT.JY1STATES.changeCurrentState(JY1SAT.JY1DATA);
  switch (JY1SAT.JY1STATES.currentState)
  {
    case PostLaunch:
      if (JY1SAT.postLaunch().fail()) goto default;
      break;

    case Telecom:
      if (JY1SAT.telecom().fail()) goto default;
      break;

    case Beaconing:
      if (JY1SAT.beaconing().fail()) goto default;
      break;

    case Recharge:

      if (JY1SAT.recharging().fail()) goto default;
      break;

    case Failsafe:
      JY1SAT.failSafe();
      break;

    default:
      JY1SAT.JY1STATES.currentState = Failsafe;
      JY1SAT.failSafe();
      break;
  }
}
