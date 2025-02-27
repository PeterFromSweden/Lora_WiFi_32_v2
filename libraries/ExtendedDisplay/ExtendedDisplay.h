#ifndef ExtendedDisplay_h
#define ExtendedDisplay_h

#include <HT_SSD1306Wire.h>

#define ADDR_OLED 0x3C
#define FREQ_OLED 50000

class ExtendedDisplay : public SSD1306Wire {
private:
    int currentY = 0;
    int lineHeight;

public:
    ExtendedDisplay(const uint8_t *fontData, int lineHeight) : SSD1306Wire(ADDR_OLED, FREQ_OLED, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED) {
		setFont(fontData, lineHeight);
	}

    void setFont(const uint8_t *fontData, int lineHeight) {
		this->lineHeight = lineHeight;
		SSD1306Wire::setFont(fontData);
	}
	
	void println(const String &text) {
		if( currentY == 0 )
		{
            clear();
		}
        drawString(0, currentY, text);
        currentY += lineHeight;
        if (currentY + lineHeight > getHeight()) {
            currentY = 0;
        }
        display();
    }

    void println(const char* text) {
        println(String(text));
    }

    void println(int number) {
        println(String(number));
    }

    void println(float number, int digits = 2) {
        println(String(number, digits));
    }

    void clearScreen() {
        clear();
        currentY = 0;
        display();
    }
};
#endif
