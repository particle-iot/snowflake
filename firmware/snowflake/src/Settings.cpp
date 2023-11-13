#include "Particle.h"
#include "Settings.h"
#include <fcntl.h>

void Settings::init( void ) {

    Log.info("Settings::init");

    int fd = open("/settings.txt", O_RDWR);

    if (fd != -1)
    {
        Log.info("Opened settings file");

        //read each line of the file out
        char buf[256];

        do 
        {
            int len = read(fd, buf, sizeof(buf)-1);

            Log.info("Read %d bytes", len);

            if (len == -1)
            {
                //error
                break;
            }
            else if (len == 0)
            {
                //EOF
                break;
            }
            else
            {
                buf[len] = 0;
                Log.info("Read %s", buf);

                //split the line - the settings is a key value pair separated by a colon
                char* key = strtok(buf, ":");

                if (key != NULL)
                {
                    char* value = strtok(NULL, ":");

                    if (value != NULL)
                    {
                        //set the setting
                        Log.info("Read setting %s:%s", key, value);

                        //add to the vector of settings
                        settings_.push_back(std::make_pair(key, value));
                    }
                }
            }
        }
        while (true);

        //close the file
        close(fd);
    }
    else
    {
        Log.info("Failed to open settings file");

        //add in the defaults
        settings_.push_back(std::make_pair("ledMode", "0"));
    }
}


void Settings::set( const String& key, const String& value ) {
    bool updated = false;

    Log.info("Settings::set %s to %s", key.c_str(), value.c_str());

    //if the setting exists in the vector replace it
    //if it doesn't exist, add it
    for (auto& setting : settings_)
    {
        if (setting.first == key)
        {
            setting.second = value;
            return;
        }
    }

    //add to the vector of settings
    if( !updated ) 
    {
        settings_.push_back(std::make_pair(key, value));
    }
}


String Settings::get( const String& key ) {
    //get the setting from the vector. If it doesn't exist, use the defaults table
    for (auto& setting : settings_)
    {
        if (setting.first == key)
        {
            Log.info("Settings::get - Returning %s for %s", setting.second.c_str(), key.c_str());
            return setting.second;
        }
    }

    return "";
}

void Settings::store( void ) {

    Log.info("Settings::store");

    //open the file
    int fd = open("/settings.txt", O_WRONLY | O_CREAT);

    if (fd != -1)
    {
        //write out each setting
        for (auto& setting : settings_)
        {
            //write out the setting
            String line = setting.first + ":" + setting.second + "\n";
            write(fd, line.c_str(), line.length());

            Log.info("Storing setting %s", line.c_str());
        }

        //close the file
        close(fd);
    }
    else
    {
        Log.info("Failed to open settings file");
    }
    
}
