#include <Wire.h> // must be ludejecfiefeceswork
#include <Keypad.h>
#include <LiquidCrystal.h>
#include <RtcDS3231.h>

#define countof(a) (sizeof(a) / sizeof(a[0]))

RtcDS3231<TwoWire> Rtc(Wire);
LiquidCrystal lcd(2, 4, A3, A2, A1, A0);

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

byte rowPins[ROWS] = {13, 12, 11, 10}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {9, 8, 7, 6}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const int bzbz = 5;  //PWM
const int backlight = 3; // PWM

enum class AlarmState {
    disabled,
    armed,
    triggered,
    disarmed
};

enum class DisplayState {
    time,
    set_alarm,
    set_password,
    alarm
};

AlarmState aState = AlarmState::armed;
int aHour = 15;
int aMinute = 12;

String password = "AD0";
String napisane = "";

DisplayState state = DisplayState::time;

void setState(DisplayState newState) {
    state = newState;
    lcd.clear();
}

void setup() {
    // put your setup code here, to run once:
    Serial.begin(9600);

    Serial.print("compiled: ");
    Serial.print(__DATE__);
    Serial.println(__TIME__);

    //--------RTC SETUP ------------
    // if you are using ESP-01 then uncomment the line below to reset the pins to
    // the available pins for SDA, SCL
    // Wire.begin(0, 2); // due to limited pins, use pin 0 and 2 for SDA, SCL

    Rtc.Begin();

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    //printDateTime(compiled);
    Serial.println();

    if (!Rtc.IsDateTimeValid()) {
        if (Rtc.LastError() != 0) {
            // we have a communications error
            // see https://www.arduino.cc/en/Reference/WireEndTransmission for
            // what the number means
            Serial.print("RTC communications error = ");
            Serial.println(Rtc.LastError());
        } else {
            // Common Causes:
            //    1) first time you ran and the device wasn't running yet
            //    2) the battery on the device is low or even missing

            Serial.println("RTC lost confidence in the DateTime!");

            // following line sets the RTC to the date & time this sketch was compiled
            // it will also reset the valid flag internally unless the Rtc device is
            // having an issue

            Rtc.SetDateTime(compiled);
        }
    }

    if (!Rtc.GetIsRunning()) {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }

    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled) {
        Serial.println("RTC is older than compile time!  (Updating DateTime)");
        Rtc.SetDateTime(compiled);
    } else if (now > compiled) {
        Serial.println("RTC is newer than compile time. (this is expected)");
    } else if (now == compiled) {
        Serial.println("RTC is the same as compile time! (not expected but all is fine)");
    }

    // never assume the Rtc was last configured by you, so
    // just clear them to your needed state
    Rtc.Enable32kHzPin(false);
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
    Serial.println("RTC Config Done");

    // LCD Display
    lcd.begin(16, 2);

    pinMode(bzbz, OUTPUT);
}

void loop() {
    // put your main code here, to run repeatedly:
    char key = keypad.getKey();
    analogWrite(backlight, 128);

    auto now = Rtc.GetDateTime();
    auto temp = Rtc.GetTemperature();

    // Check alarm
    if (aState == AlarmState::armed && now.Hour() == aHour && now.Minute() == aMinute) {
        Serial.println("Triggering");
        aState = AlarmState::triggered;
        setState(DisplayState::alarm);
    } else if (aState == AlarmState::disarmed && (now.Hour() != aHour || now.Minute() != aMinute)) {
        Serial.println("Rearming");
        aState = AlarmState::armed;
    }

    // Display stuff
    if (state == DisplayState::time) {
        vypisCas(now, temp);
    } else if (state == DisplayState::alarm) {
        vypisBudik(key, now);
    } else if (state == DisplayState::set_alarm) {

    } else if (state == DisplayState::set_password) {

    }
    digitalWrite(bzbz, (aState == AlarmState::triggered));
}

String get_date(const RtcDateTime &dt) {
    char datestring[20];

    snprintf_P(datestring,
        countof(datestring),
        PSTR("%02u.%02u.%04u"),
        dt.Day(),
        dt.Month(),
        dt.Year()
    );
    return String(datestring);
}

String get_time(const RtcDateTime &dt, bool sec=true) {
    char datestring[20];
    if (sec) {
        snprintf_P(
            datestring,
            countof(datestring),
            PSTR("%02u:%02u:%02u"),
            dt.Hour(),
            dt.Minute(),
            dt.Second()
        );
    } else {
        snprintf_P(
            datestring,
            countof(datestring),
            PSTR("%02u:%02u"),
            dt.Hour(),
            dt.Minute()
        );
    }

    return String(datestring);
}

void vypisCas(const RtcDateTime &now, const RtcTemperature &temp) {
    auto time_str = get_time(now);
    auto date_str = get_date(now);

    lcd.setCursor(0, 0);
    lcd.write(time_str.c_str());
    lcd.setCursor(10, 0);
    lcd.print(temp.AsFloatDegC(), 1);
    lcd.write("\xDF" "C");

    lcd.setCursor(0, 1);
    lcd.write(date_str.c_str());
}

void vypisBudik(char key, const RtcDateTime &now) {
    auto time_str = get_time(now, false);

    lcd.setCursor(0, 0);
    lcd.write("Password:");
    lcd.setCursor(11, 0);
    lcd.write(time_str.c_str());

    if (key == '#') {
        if (password == napisane) {
            setState(DisplayState::time);
            aState = AlarmState::disarmed;
        } else {
            lcd.clear();
        }
        napisane = "";

    } else if (key) {
        napisane += key;
    }

    lcd.setCursor(0, 1);
    lcd.write(napisane.c_str());
}
