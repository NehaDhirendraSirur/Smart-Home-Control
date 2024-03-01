#define F_CPU 8000000UL        /* Define CPU Frequency e.g. here 8MHz */
#include <avr/io.h>            /* Include AVR std. library file */
#include <util/delay.h>        /* Include inbuilt defined Delay header file */

#define LCD_Data_Dir DDRD      /* Define LCD data port direction */
#define LCD_Data_Port PORTD    /* Define LCD data port */
#define LCD_Command_Dir DDRB   /* Define LCD command port direction register */
#define LCD_Command_Port PORTB /* Define LCD data port */
#define RS PB0                /* Define Register Select (data/command reg.)pin */
#define RW PB1                /* Define Read/Write signal pin */
#define EN PB2                /* Define Enable signal pin */

#define LED1_PORT PORTC    // Define LED1 port
#define LED1_PIN PC0       // Define LED1 pin
#define LED2_PORT PORTC    // Define LED2 port
#define LED2_PIN PC1       // Define LED2 pin

#define KEY_PRT  PORTA    // Keyboard PORT
#define KEY_DDR  DDRA    // Keyboard DDR
#define KEY_PIN  PINA    // Keyboard PIN

unsigned char keypad[4][4] = {
	{'.', '-', '=', '/'},
	{'1', '4', '7', '*'},
	{'2', '5', '8', '0'},
	{'3', '6', '9', '#'}
};

unsigned char colloc, rowloc;
unsigned char led1_state = 0; // LED1 state (0 = OFF, 1 = ON)
unsigned char led2_state = 0; // LED2 state (0 = OFF, 1 = ON)
unsigned char fan_state = 0; // Fan state (0 = OFF, 1 = ON)

void LCD_Command(unsigned char cmnd)
{
	LCD_Data_Port = cmnd;
	LCD_Command_Port &= ~(1 << RS);    /* RS=0 command reg. */
	LCD_Command_Port &= ~(1 << RW);    /* RW=0 Write operation */
	LCD_Command_Port |= (1 << EN);     /* Enable pulse */
	_delay_us(1);
	LCD_Command_Port &= ~(1 << EN);
	_delay_ms(3);
}

void LCD_Char(unsigned char char_data) /* LCD data write function */
{
	LCD_Data_Port = char_data;
	LCD_Command_Port |= (1 << RS);     /* RS=1 Data reg. */
	LCD_Command_Port &= ~(1 << RW);    /* RW=0 write operation */
	LCD_Command_Port |= (1 << EN);     /* Enable Pulse */
	_delay_us(1);
	LCD_Command_Port &= ~(1 << EN);
	_delay_ms(1);
}

void LCD_Init(void)         /* LCD Initialize function */
{
	LCD_Command_Dir = 0xFF;     /* Make LCD command port direction as o/p */
	LCD_Data_Dir = 0xFF;        /* Make LCD data port direction as o/p */
	_delay_ms(20);              /* LCD Power ON delay always >15ms */

	LCD_Command(0x38);          /* Initialization of 16X2 LCD in 8bit mode */
	LCD_Command(0x0C);          /* Display ON Cursor OFF */
	LCD_Command(0x01);          /* Clear display */
	LCD_Command(0x80);          /* Cursor at home position */
}

void LCD_String(char *str)      /* Send string to LCD function */
{
	int i = 0;
	for (i = 0; str[i] != 0; i++)    /* Send each char of string till the NULL */
	{
		LCD_Char(str[i]);
	}
}

void LCD_Clear()
{
	LCD_Command(0x01);       /* clear display */
	LCD_Command(0x80);       /* cursor at home position */
}

void initializePorts()
{
	DDRC |= (1 << LED1_PIN) | (1 << LED2_PIN);    // Set LED pins as output
	PORTC &= ~((1 << LED1_PIN) | (1 << LED2_PIN)); // Turn off LEDs initially

	// Motor Driver Configuration
	DDRB |= (1 << PB3) | (1 << PB4) | (1 << PB5) | (1 << PB6); // Set motor control pins as output
	PORTB &= ~((1 << PB3) | (1 << PB4) | (1 << PB5) | (1 << PB6)); // Set motor control pins low initially
}

void motorControl(uint8_t controlBits)
{
	PORTB = (PORTB & 0x8F) | (controlBits << 4);
}

char keyfind()
{
	while (1)
	{
		KEY_DDR = 0xF0;           /* set port direction as input-output */
		KEY_PRT = 0xFF;
		/* now check for rows */
		KEY_PRT = 0xEF;            /* check for pressed key in 1st row */
		colloc = (KEY_PIN & 0x0F);
		if (colloc != 0x0F)
		{
			rowloc = 0;
			break;
		}

		KEY_PRT = 0xDF;        /* check for pressed key in 2nd row */
		colloc = (KEY_PIN & 0x0F);
		if (colloc != 0x0F)
		{
			rowloc = 1;
			break;
		}

		KEY_PRT = 0xBF;        /* check for pressed key in 3rd row */
		colloc = (KEY_PIN & 0x0F);
		if (colloc != 0x0F)
		{
			rowloc = 2;
			break;
		}

		KEY_PRT = 0x7F;        /* check for pressed key in 4th row */
		colloc = (KEY_PIN & 0x0F);
		if (colloc != 0x0F)
		{
			rowloc = 3;
			break;
		}
	}

	if (colloc == 0x0E)
	{
		LCD_Clear();
		return (keypad[rowloc][0]);
	}
	else if (colloc == 0x0D)
	{
		LCD_Clear();
		return (keypad[rowloc][1]);
	}
	else if (colloc == 0x0B)
	{
		LCD_Clear();
		return (keypad[rowloc][2]);
	}
	else
	{
		LCD_Clear();
		return (keypad[rowloc][3]);
	}
}

void toggleFan() {
	if (fan_state == 0) {
		LCD_String("FAN is ON     ");
		fan_state = 1;
		motorControl(0b01); // Rotate motor in one direction
		} else {
		LCD_String("FAN is OFF    ");
		fan_state = 0;
		motorControl(0b00); // Stop motor
	}
}

int main(void)
{
	LCD_Init();
	initializePorts();
	LCD_String("1,2-LED 5-FAN");
	LCD_Command(0xC0);
	LCD_String("Press 1,2 or 5");
	while (1)
	{
		char key = keyfind();
		LCD_Command(0xC0);
		//LCD_Char(key);       /* Display which key is pressed */
		switch (key)
		{
			case '1':
			if (led1_state == 0)
			{
				LCD_String("LED1 is ON    ");
				led1_state = 1;
				PORTC |= (1 << LED1_PIN);
			}
			else
			{
				LCD_String("LED1 is OFF   ");
				led1_state = 0;
				PORTC &= ~(1 << LED1_PIN);
			}
			break;
			case '2':
			if (led2_state == 0)
			{
				LCD_String("LED2 is ON    ");
				led2_state = 1;
				PORTC |= (1 << LED2_PIN);
			}
			else
			{
				LCD_String("LED2 is OFF   ");
				led2_state = 0;
				PORTC &= ~(1 << LED2_PIN);
			}
			break;
			case '5': toggleFan();
			break;
			default:
			break;
		}
	}
}
