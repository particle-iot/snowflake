#include "Particle.h"
#include "Settings.h"
#include <fcntl.h>

void Settings::init( void ) {

    Log.info("Settings::init");

    int fd = open("/settings.txt", O_RDWR);

    bool fileExistsButIsBad = false;

    if (fd != -1)
    {
        Log.info("Opened settings file");

        //read the entore file out, up to 512 bytes
        char *buf = (char *)calloc(1, 512);

        int len = read(fd, buf, 512);  

        //close the file
        close(fd);

        Log.info("Read %d bytes", len);

        if (len == -1)
        {
            fileExistsButIsBad = true;
        }
        else if (len == 0)
        {

        }
        else
        {
            char* p = buf;

            //loop over the buffer, extracting a string at a time (terminated by \n)
            while( fileExistsButIsBad == false )
            {
                //find the next \n
                char* nl = strchr(p, '\n');

                if (nl == NULL)
                {
                    break;
                }

                //terminate the string by replacing the \n with a \0
                *nl = 0;

                Log.info("Read string: %s", p);

                //split the line - the settings is a key value pair separated by a colon
                char* key = strtok(p, ":");

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
                    else
                    {
                        Log.info("Failed to read: value");
                        fileExistsButIsBad = true;
                    }
                }
                else
                {
                    Log.info("Failed to read: key");
                    fileExistsButIsBad = true;
                }

                //advance p to the next line
                p = nl + 1;
            }
        }

        free( buf );
    }
    else
    {
        Log.info("Failed to open settings file");

        //add in the defaults
        settings_.push_back(std::make_pair("ledMode", "1"));
    }

    //if the file exists but is bad, delete it
    if (fileExistsButIsBad)
    {
        Log.info("Deleting bad settings file");

        unlink("/settings.txt");
    }

    //register for 2 callbacks
    //settingsReadFunction is a instance of this class and a pointer to the function
    //cast it appropriately
    const bool success = Particle.function("settingsRead", std::bind(&Settings::settingsReadFunction, this, _1) );
    Log.info("Particle.function(settingsRead) %s", success ? "registered OK" : "failed to register");

    const bool success2 = Particle.function("settingsWrite", std::bind(&Settings::settingsWriteFunction, this, _1) );
    Log.info("Particle.function(settingsWrite) %s", success2 ? "registered OK" : "failed to register");
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
            int dataWritten = write(fd, line.c_str(), line.length());

            if( dataWritten != line.length() )
            {
                Log.info("Failed to write setting %s", line.c_str());
            }

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


//a particle function that allows the settings to be read / written
int Settings::settingsReadFunction(String extra)
{
    //get the value
    String value = get(extra);

    Log.info("Settings::settingsReadFunction - %s:%s", extra.c_str(), value.c_str());

    //publish the value
    Particle.publish("settingsReadReply", value, PRIVATE);

    return 0;
};


int Settings::settingsWriteFunction(String extra)
{
    //split extra by : into key and value
    int colonIndex = extra.indexOf(':');

    if( colonIndex > 0 ) {
        String key = extra.substring(0, colonIndex);
        String value = extra.substring(colonIndex + 1);

        Log.info("Settings::settingsWriteFunction - %s:%s", key.c_str(), value.c_str());

        //set the value
        set(key, value);
        store();

        //publish the value
        Particle.publish("settingsWriteReply", "OK", PRIVATE);
    }
    else {
        //publish the value
        Particle.publish("settingsWriteReply", "ERROR", PRIVATE);

        Log.info("Settings::settingsWriteFunction - ERROR (%s)", extra.c_str());
    }

    return 0;
};
