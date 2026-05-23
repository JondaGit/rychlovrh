#pragma once

// WiFi
// Note: iPhone hotspots broadcast with a typographic apostrophe (U+2019, ’),
// not a straight ASCII apostrophe. If connection fails, try the other variant.
#define WIFI_SSID     "Jonáš’s iPhone"
#define WIFI_PASSWORD "jemnicejenejvice"

// Firebase project
#define FIREBASE_API_KEY    "AIzaSyDBGL9G0Zhgsbh10dUFtPZ9SqTb1WM4xa8"
#define FIREBASE_PROJECT_ID "svistivrh"
#define FIREBASE_AUTH_DOMAIN "svistivrh.firebaseapp.com"

// Device auth (unused while DB is in open/test mode; ready for when rules require auth)
// TODO: fill DEVICE_EMAIL with the address used when creating the Firebase user
#define DEVICE_EMAIL    ""
#define DEVICE_PASSWORD "joujoujou"
#define DEVICE_UID      "PlgfbYkQRfgUmNgrZZnUy8rfu6W2"
