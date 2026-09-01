                  ┌──────────────┐
                  │  Vehicle HW  │
                  └──────┬───────┘
                         │
              ┌──────────▼──────────┐
              │    vehicleState     │
              └──────────┬──────────┘
                         │
                 telemetry semaphore
                         │
                ┌────────▼────────┐
                │  Telemetry Task │
                └────────┬────────┘
                         │
                    loraTXQueue
                         │
                ┌────────▼────────┐
                │    LoRa TX      │
                └────────┬────────┘
                         │
                     LoRa radio
                         │
                     LoRa RX
                         │
                ┌────────▼────────┐
                │    LoRa RX      │
                └───────┬─┬───────┘
                        │ │
               ┌────────┘ └────────┐
               ▼                   ▼
         displayQueue         serialQueue
