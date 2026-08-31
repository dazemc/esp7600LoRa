#include "display.h"
#include "types.h"
#include "events.h"
#include "event_bus.h"

DisplayData displayData{};

static ssd1306_handle_t displayHandle = nullptr;
static int oledX = 0;
static int oledY = 0;

static i2c_master_bus_config_t bus_config = {
    .i2c_port = I2C_NUM_0,
    .sda_io_num = OLED_SDA,
    .scl_io_num = OLED_SCL,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .intr_priority = 0,
    .trans_queue_depth = 0,
    .flags =
        {
            .enable_internal_pullup = true,
            .allow_pd = false,
        },
};

void initDisplay() {
  i2c_master_bus_handle_t bus_handle;

  ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

  ssd1306_config_t cfg{};

  cfg.bus = SSD1306_I2C;
  cfg.width = OLED_WIDTH;
  cfg.height = OLED_HEIGHT;

  cfg.iface.i2c.port = I2C_NUM_0;
  cfg.iface.i2c.addr = 0x3C;
  cfg.iface.i2c.rst_gpio = OLED_RST;

  ESP_ERROR_CHECK(ssd1306_new_i2c(&cfg, &displayHandle));

  displayData.disp = displayHandle;
  displayData.OledX = oledX;
  displayData.OledY = oledY;
}

void displayTask(void *arg) {
  EventLoRa event;

  while (true) {
    if (xQueueReceive(displayQueue, &event, portMAX_DELAY)) {
      switch (event.type) {

      case EVENT_LORA_SEND: {
        const char *prepend = "Sending:\nVoltage: ";

        snprintf(displayData.message, sizeof(displayData.message), "%s%.4f",
                 prepend, event.loraSend.vehicle.voltageData.battery);

        ssd1306_clear(displayHandle);

        ssd1306_draw_text(displayHandle, oledX, oledY, displayData.message,
                          true);

        ssd1306_display(displayHandle);

        break;
      }

      default:
        Serial.println("Waiting to display...");
        break;
      }
    }
  }
}
