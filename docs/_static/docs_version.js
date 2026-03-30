var DOCUMENTATION_VERSIONS = {
    DEFAULTS: { has_targets: false,
                supported_targets: [ "esp32c3", "esp32c6" ]
              },
    VERSIONS: [
      { name: "latest", has_targets: true, supported_targets: [ "esp32c3", "esp32c6"] },
      { name: "release/v1.0.0", has_targets: true, supported_targets: [ "esp32c3"] },
    ],
    IDF_TARGETS: [
       { text: "ESP32-C3", value: "esp32c3"},
       { text: "ESP32-C6", value: "esp32c6"},
    ]
};
