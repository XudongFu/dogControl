#include <Servo.h>
#include <SoftwareSerial.h> 
#include <string.h>
#include <Stepper.h>
#include <Servo.h>
using namespace std;

class wifi
{
public:
	wifi(int rx, int wx);
	~wifi();
	bool connect(String ssid, String password);
	bool sendData(String data);
	void setHandle(void(*fun)(String));
	void Loop();
	bool connectTCP(String ip, String port);
	bool conn();
	
private:
	SoftwareSerial* mySerial; // RX, TX
	bool connected = false;
	void(*handle)(String) = 0;
	String ssid;
	String password;
	const int size = 100;
	char* buffer;
	char* doubleBuffer;
	int rx;
	int wx;
	const String newline = "/r/n";
	String receiveback(int);
	void test(String);
	void ReadBackData(char* data, int bufferSize, int timeout);
	bool CheckReceiveOk(String);
	bool ReceiveOk(int timeout);
	bool debug = true;
	int FailTime = 0;
	const int FailCount = 20;
	bool ReveiveStr(int timeout, String str);
	bool IsConnectToWIFI();

	const int HardWareSize = 2;
	int CurrentIndex = 0;

};


#pragma region MyRegion


bool wifi::IsConnectToWIFI()
{
	return false;
}

bool wifi::ReceiveOk(int timeout)
{
	for (int i = 0; i < size; i++)
	{
		buffer[i] = '\0';
	}
	unsigned long stop = millis() + timeout;
	int index = 0;
	do {
		if (mySerial->available())
		{
			char c = mySerial->read();
			buffer[index++] = c;
			String temp(buffer);
			if (temp.lastIndexOf("OK") > 0)
			{
				return true;
			}
		}
	} while (millis() < stop && index < size - 1);
	return false;
}

bool wifi::ReveiveStr(int timeout, String str)
{
	for (int i = 0; i < size; i++)
	{
		buffer[i] = '\0';
	}
	unsigned long stop = millis() + timeout;
	int index = 0;
	do {
		if (mySerial->available())
		{
			char c = mySerial->read();
			buffer[index++] = c;
			String temp(buffer);
			if (temp.lastIndexOf("OK") > 0)
			{
				return true;
			}
		}
	} while (millis() < stop && index < size - 1);
	return false;
}


bool wifi::CheckReceiveOk(String order)
{
	if (order.length() == 0)
	{
		return false;
	}
	if (order.length() == 1)
	{
		return true;
	}
	int half = order.length() / 2;
	int length = order.length();
	for (size_t i = 0; i < half; i++)
	{
		if (order[i] != order[length - 1 - i])
			return false;
	}
	return true;
}

void wifi::test(String Order)
{
	Serial.println(Order);
	mySerial->println(Order);
	receiveback(100);
}

void wifi::ReadBackData(char* data, int bufferSize, int timeout)
{
	unsigned long stop = millis() + timeout;
	int index = 0;
	do {
		if (mySerial->available())
		{
			char c = mySerial->read();
			data[index++] = c;
		}
	} while (millis() < stop && index < bufferSize - 1);
	data[index++] = '\0';
}


wifi::wifi(int rx, int wx)
{
	this->rx = rx;
	this->wx = wx;
	mySerial = new SoftwareSerial(rx, wx);
	mySerial->begin(115200);
	buffer = new char[size];
	buffer[size - 1] = '\0';
}

wifi::~wifi()
{
	delete[] buffer;
	delete[] doubleBuffer;
}

bool wifi::connect(String ssid, String password)
{
	mySerial->println("AT+CWMODE=1");
	receiveback(1000);
	String order = "AT+CWJAP=\"" + ssid + "\",\"" + password + "\"";
	if (debug)
		Serial.println(order);
	mySerial->println(order);
	connected = ReceiveOk(10000);
	return connected;
}

bool wifi::sendData(String data)
{
	String order = "AT+CIPSEND=2,";
	mySerial->print(order);
	mySerial->println(data.length());
	ReveiveStr(100, ">");
	mySerial->println(data);
	bool result = ReceiveOk(100);
	if (result == false)
	{
		if (debug)
			Serial.println("wifi not connected");
		if (FailTime < FailCount)
		{
			FailTime++;
		}
		else
		{
			while (conn() == false)
			{
				if (debug)
				{
					Serial.println("try reconnect to wifi");
				}
			}
		}
	}
	else
	{
		return true;
	}
}

void wifi::setHandle(void(*fun)(String))
{
	this->handle = fun;
}

void wifi::Loop()
{
	String command = receiveback(100);
	if (command != NULL && command.length() > 0)
	{
		Serial.println("received: " + command);
		int index = command.lastIndexOf(':');
		if (index > 0 && index < command.length() - 1)
		{
			command = command.substring(index+1);
			index = command.lastIndexOf(',');
			if (index > 0 && index < command.length() - 1)
			{
				String aa = command.substring(0, index);
				int footPin = StringToint(aa);
				String bb = command.substring(index + 1, command.length());
				int degree = StringToint(bb);
				SendCommand(footPin, degree);
				Serial.print("command: " + aa + "   " + bb + ":");
				Serial.print(footPin);
				Serial.print("   ");
				Serial.print(degree);
				Serial.println("");
			}

		}
		else
		{
			Serial.println("command errot: " + command);
		}
	}
	else
	{
		if (command.indexOf("CLOSED") > 0)
		{


		}
	}
}

bool wifi::connectTCP(String ip, String port)
{
	mySerial->println("AT+CIPMUX=1");
	String order = "AT+CIPSTART=2,\"TCP\",\"" + ip + "\"," + port;
	if (debug)
		Serial.println(order);
	mySerial->println(order);
	String res = receiveback(2000);
	if (debug)
		Serial.println("read data is " + res);
	if (res.lastIndexOf("OK") > 0 || res.lastIndexOf("READY") > 0)
	{
		return true;
	}
	else
	{
		return  false;
	}
}

String wifi::receiveback(int timeout = 100)
{
	ReadBackData(buffer, size, timeout);
	String res(buffer);
	return res;
}

void HandleOrder(String order)
{
	Serial.println(order);
}

bool wifi::conn()
{
	int tryCount = 3;
	int tryTime = 0;
	Serial.println("hardware serial!");
	pinMode(LED_BUILTIN,OUTPUT);
	bool res = false;
	while (res == false && tryTime < tryCount)
	{
		tryCount++;
		res = connect("Danke_803", "wifi.danke.life");
	}
	if (res)
	{
		Serial.println("connect to network successful");
		bool connect = false;
		tryTime = 0;
		while (connect == false && tryTime < tryCount)
		{
			tryCount++;
			connect = connectTCP("192.168.199.243", "1234");
			digitalWrite(LED_BUILTIN, HIGH);
		}
		if (connect)
		{
			FailTime = 0;
			Serial.println("connect to ap successful");
		}
		else
		{
			Serial.println("connect to ap failed");
		}
	}
	else
	{
		Serial.println("connect to network failed");
	}
	setHandle(HandleOrder);
}

#pragma endregion

int StringToint(String str)
{
	if (str.length()==0)
	{
		return -1;
	}
	int res = 0;
	for (uint8_t i=0;i<str.length();i++)
	{
		if (str[i]>='0' && str[i]<='9')
		{
			res *= 10;
			res += str[i] - '0';
		}
		else
		{
			return res;
		}
	}
	return res;
}

Servo* myservo[8];
int direction[8];

void SendCommand(uint8_t footpin,int Degree)
{
	if (footpin >= 0 && footpin < 8 && Degree >=0 && Degree<90)
	{
		myservo[footpin]->write(90+direction[footpin]*Degree);
	}
}

void Stand()
{
	for (uint8_t i = 0; i < 8; i++)
	{
		SendCommand(i, 0);
	}
	delay(3000);
}

wifi mywifi(11, 10);

int maxDegree = 45;

void setup() {
	Serial.begin(115200);
	for (int i = 2; i < 8; i++)
	{
		myservo[i] = new Servo();
		myservo[i]->attach(i);
	}
	digitalWrite(LED_BUILTIN, LOW);
	myservo[0] = new Servo();
	myservo[0]->attach(8);
	myservo[1] = new Servo();
	myservo[1]->attach(9);
	direction[0] = 1;
	direction[1] = 1;
	direction[2] = -1;
	direction[3] = -1;
	direction[4] = 1;
	direction[5] = 1;
	direction[6] = 1;
	direction[7] = -1;
	//Stand();
	while (!Serial) { ; }
	mywifi.conn();
}

void loop()
{
	mywifi.Loop();

}