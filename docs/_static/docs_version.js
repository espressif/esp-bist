var DOCUMENTATION_VERSIONS = {
    DEFAULTS: { has_targets: false,
                supported_targets: [ "esp32c3", "esp32c5", "esp32c6", "esp32c61", "esp32h2", "esp32p4" ]
              },
    VERSIONS: [
      { name: "latest", has_targets: true, supported_targets: [ "esp32c3", "esp32c5", "esp32c6", "esp32c61", "esp32h2", "esp32p4"] },
      { name: "release/v1.0.0", has_targets: true, supported_targets: [ "esp32c3"] },
    ],
    IDF_TARGETS: [
       { text: "ESP32-C3", value: "esp32c3"},
       { text: "ESP32-C5", value: "esp32c5"},
       { text: "ESP32-C6", value: "esp32c6"},
       { text: "ESP32-C61", value: "esp32c61"},
       { text: "ESP32-H2", value: "esp32h2"},
       { text: "ESP32-P4", value: "esp32p4"},
    ]
};
