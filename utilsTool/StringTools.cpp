#include <Arduino.h>
String *LedSystemInfoParser(String s, int infoSections)
{   
    s.trim();
    String *info = new String[infoSections];
    int index = 0;
    int startPos = 0;

    while (true)
    {
        int endPos = s.indexOf(",", startPos);
        if (endPos == -1)
        {
            info[index] = s.substring(startPos);
            break;
        }
        info[index] = s.substring(startPos, endPos);
        index++;
        startPos = endPos + 1;
    }

    return info;
}

String *ledOpHourParser(String s, int infoSections)
{
    s.trim();
    String *hours = new String[infoSections];
    int pos = s.indexOf("-");
    hours[0] = s.substring(0, pos);
    hours[1] = s.substring(pos+1);
    return hours;
}

boolean isNumericString(String s){
    s.trim();
    for(unsigned int  i= 0; i<s.length(); i++){
        if( !isdigit(s[i]) ) return false;
    }
    return true;
}