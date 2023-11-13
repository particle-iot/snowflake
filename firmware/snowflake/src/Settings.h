#pragma once

const String SETTINGS_FILE = "settings.json";

class Settings {
    public:

      //initial load in
      void init( void );

      //settings
      void set( const String& key, const String& value );
      String get( const String& key );

      //store the settings
      void store( void );

    private:

      //a defaults setting table
      // ledMode:0

      //the settings - a vector of key value pairs
      std::vector<std::pair<String, String>> settings_;
};
