/*
* Copyright (C) 2014  Arjun Sreedharan
* License: GPL version 2 or higher http://www.gnu.org/licenses/gpl.html
*/

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef char int8;
typedef short int16;
typedef unsigned int uint32;

#include "constants.h"
#include "keyboard_map.h"

/* there are 25 lines each of 80 columns; each element takes 2 bytes */
#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e

#define KERNEL_CODE_SEGMENT_OFFSET 0x08

#define ENTER_KEY_CODE 0x1C

//максимально возможна длина вводимого сообщения
#define max_length 80

//максимальное возможное количество введенных строк
#define max_count_entered_string 5

extern unsigned char keyboard_map[128];
extern void keyboard_handler(void);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
//extern void load_idt(unsigned long *idt_ptr);
extern void restart(void);

// Это две разные реализации одной и той же штуки
extern void exit(void);
extern void exit_1(void);

/* current cursor location */
unsigned int current_loc = 0;
/* video memory begins at address 0xb8000 */
char *vidptr = (char*)0xb8000;

// длина введенного сообщения
int length_entered_sentense = 0;
// то, где будет храниться сообщение (выделяем память)
char msg [max_length];
// место для хранения имени пользователя
char my_name [max_length - 14];

// кол-во введенных строк (чтобы своевременно очищать экран)
int count_entered_string = 0;

//текущая позиция курсора
int CURSOR_POSITION = 0;

// текущее положение курсора
uint8 row_current = 0;
uint8 col_current = 0;


// необходимая для настройки драйверов общения процессора с клавиатурой структра данных
struct IDT_entry {
	unsigned short int offset_lowerbits;
	unsigned short int selector;
	unsigned char zero;
	unsigned char type_attr;
	unsigned short int offset_higherbits;
//};
} __attribute__((packed));


struct idt_ptr {
	unsigned short limit;
	unsigned long base;
} __attribute__((packed));

extern void load_idt(struct idt_ptr *idt_ptr);


// объявляем массив таких структур
struct IDT_entry IDT[IDT_SIZE];

// это сложно регистрозависимая (именно intel'овская) функция настройки драйверов для x86 процессоров с клавиатурой	
void idt_init(void)
{
	unsigned long keyboard_address;
	struct idt_ptr idt_ptr;
	//unsigned long idt_address;
	//unsigned long idt_ptr[2];

	/* populate IDT entry of keyboard's interrupt */
	keyboard_address = (unsigned long)keyboard_handler;
	IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
	IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
	IDT[0x21].zero = 0;
	IDT[0x21].type_attr = INTERRUPT_GATE;
	IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;

	/*     Ports
	*	 PIC1	PIC2
	*Command 0x20	0xA0
	*Data	 0x21	0xA1
	*/

	/* ICW1 - begin initialization */
	write_port(0x20 , 0x11);
	write_port(0xA0 , 0x11);

	/* ICW2 - remap offset address of IDT */
	/*
	* In x86 protected mode, we have to remap the PICs beyond 0x20 because
	* Intel have designated the first 32 interrupts as "reserved" for cpu exceptions
	*/
	write_port(0x21 , 0x20);
	write_port(0xA1 , 0x28);

	/* ICW3 - setup cascading */
	write_port(0x21 , 0x00);
	write_port(0xA1 , 0x00);

	/* ICW4 - environment info */
	write_port(0x21 , 0x01);
	write_port(0xA1 , 0x01);
	/* Initialization finished */

	/* mask interrupts */
	write_port(0x21 , 0xff);
	write_port(0xA1 , 0xff);

	/* fill the IDT descriptor */
	//idt_address = (unsigned long)IDT ;
	//idt_ptr[0] = (sizeof (struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
	//idt_ptr[1] = idt_address >> 16 ;
	idt_ptr.limit = sizeof(IDT);
	idt_ptr.base = (unsigned long)IDT;

	//load_idt(idt_ptr);
	load_idt(&idt_ptr);
}

// еще одна функция необходимая для инициализации клавиатуры
void kb_init(void)
{
	/* 0xFD is 11111101 - enables only IRQ1 (keyboard)*/
	write_port(0x21 , 0xFD);
}



/*

*** ПРИМЕЧАНИЕ !!! ***

 подробнее обо всех вышеперечисленных регистрозависимых функция настройки общения процессора и клаивтуры нужно смотреть 2-ю статью  из Интернета (папка содержащая данные статьи лежит рядом с проектом)

*/

// установка видимого курсора
void set_current_position_visibale_cursor(void)
{
	
	CURSOR_POSITION = current_loc/2;
	int current_row = CURSOR_POSITION/COLUMNS_IN_LINE;
	int current_col = CURSOR_POSITION % COLUMNS_IN_LINE;
	set_cursor(current_row, current_col);
}

void kprint(const char *str)
{
	unsigned int i = 0;
	while (str[i] != '\0') {
		vidptr[current_loc++] = str[i++];
		CURSOR_POSITION++;
		vidptr[current_loc++] = 0x07;
	}
	
	set_current_position_visibale_cursor();
}


// та же самая функция "void kprint(void), только выводит все другим цветом"

// color: 0x<цвет_фона><цвет текста>

// Доступные цвета: 0 - Black, 1 - Blue, 2 - Green, 3 - Cyan, 4 - Red, 5 - Magenta, 6 - Brown, 7 - Light Grey, 8 - Dark Grey, 9 - Light Blue, 10/a - Light Green, 11/b - Light Cyan, 12/c - Light Red, 13/d - Light Magenta, 14/e - Light Brown, 15/f – White.

void kprint_color(const char *str, int color)
{
	unsigned int i = 0;
	while (str[i] != '\0') {
		vidptr[current_loc++] = str[i++];
		CURSOR_POSITION++;
		vidptr[current_loc++] = color;
	}
	
	set_current_position_visibale_cursor();
}


void kprint_newline(void)
{
	unsigned int line_size = BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE;
	current_loc = current_loc + (line_size - current_loc % (line_size));
	set_current_position_visibale_cursor();
}

// это старая функция очистки экрана (новая лучше), но эту удалять пока не будем
/*
void clear_screen(void)
{
	unsigned int i = 0;
	while (i < SCREENSIZE) {
		vidptr[i++] = ' ';
		vidptr[i++] = 0x07;
	}
}
*/

// Функция помощник (очищает строку-буффер, в которой хранится вводимое имя функции)
void clear_buffer_string(void)
{
	// очищаем строку ------------------
	int i=0;
	for (i; i < max_length; i++)
		msg[i] = '\0';
	length_entered_sentense = 0;
	// ---------------------------------	
	}

// Моя реализация проверки эквивалентности строк
int strcmp(const char* str1, const char* str2)
{

	int result = 1;
	int count = 0;
	while (str1[count] != '\0')
	{
		if (str1[count] != str2[count])
		{
			result = 0;
			return result;
		}
		count++;
	}
	
	if (str2[count] != '\0')
	{
		result = 0;
		return result;
	}
	
	return result;
}


//Функция записи имени пользователя в память
void read_user_name(const char* sentence)
{
	int i=0;
	for (i; i < max_length - 14; i++)
		my_name[i] = '\0';
	
	// записываем в переменную my_name введенный пользователем аргумент (его имя)
	int count = 14;
	while (sentence[count] != '\0')
	{
		my_name[count-14] = sentence[count];
		count++;
	}
	
	my_name[count] = '\0';
	
	// выведем имя, которое ввел пользователь
	kprint_newline();
	const char *str = "Your name save as: ";
	kprint(str);
	kprint(my_name);
	kprint_newline();
	return;
}


// Функция вывода имени текущего пользователя
void print_user_name(void)
{
	kprint_newline();
	const char *str = "Your name is: ";
	kprint(str);
	kprint(my_name);
	kprint_newline();
	return;
}


// Функция вывода справочника (help)
void print_help(void)
{
	kprint_newline();
	kprint_newline();
	const char *str0 = "      <==== HELP ====>";
	kprint(str0);
	kprint_newline();
	kprint_newline();
	const char *str = "Your may enter next commands: ";
	kprint(str);
	kprint_newline();
	kprint("        1) \"enter my name <user_name>\"");
	kprint_newline();
	kprint("        2) \"print my name\"");
	kprint_newline();
	kprint("        3) \"restart\"");	
	kprint_newline();
	kprint("        4) \"exit\"");
	kprint_newline();
	kprint("        5) \"logo\"");
	kprint_newline();
	kprint("        6) \"clear\"");
	kprint_newline();
	kprint("        ... ");
	kprint_newline();
	kprint("        ... ");	
	kprint_newline();
	kprint("        features will be added in the future ... ");
	kprint_newline();
	kprint_newline();
	return;
}


// Функция рисования логотипа ОС
void print_logo_os(void)
{	
	clear_screen();
	kprint_newline();
	kprint_newline();
	const char *str0 = "        < ==== LOGO ====> ";
	kprint(str0);
	kprint_newline();
	kprint_newline();
	
	// будем печатать наш логотип зелеными буквами на черном фоне
	
	int color = 0x02;
	const char *s0 = "                          ";
	kprint_color(s0, color);
	kprint_newline();
	const char *s1 = "              ##  ##      ";
	kprint_color(s1, color);
	kprint_newline();
	const char *s2 = "           ##        ##   ";
	kprint_color(s2, color);
	kprint_newline();
	const char *s3 = "          ##  #####   ##  ";
	kprint_color(s3, color);
	kprint_newline();
	const char *s4 = "             #######      ";
	kprint_color(s4, color);
	kprint_newline();
	const char *s5 = "           ##########     ";
	kprint_color(s5, color);
	kprint_newline();
	const char *s6 = "          ############    ";
	kprint_color(s6, color);
	kprint_newline();
	const char *s7 = "           ##########     ";
	kprint_color(s7, color);
	kprint_newline();
	const char *s8 = "            ########      ";
	kprint_color(s8, color);
	kprint_newline();
	const char *s9 = "             ######       ";
	kprint_color(s9, color);
	kprint_newline();
	const char *s10 = "                          ";
	kprint_color(s10, color);
	kprint_newline();
	kprint_newline();
	
	color = 0x04; // красные буквы на черном фоне
	const char *str = "        @@@ SS_OS_45-ka @@@ ";
	kprint_color(str, color);
	kprint_newline();
	kprint_newline();
	
	color = 0x01; // голубые буквы на черном фоне
	const char *str1 = "         Date: 15.06.2020 ";
	kprint_color(str1, color);
	kprint_newline();
	kprint_newline();
	
	//выполняем условия для очистки экрана
	count_entered_string = max_count_entered_string - 2; //  (-2) - это чтобы сразу все не очистилось
	
	
	color = 0x0f; // белые буквы на черном фоне
	const char *str2 = "For clearing LOGO simple press \"Enter\" ";
	kprint_color(str2, color);
	kprint_newline();
	
	return;
}



// Функция обрабатывающая введенные сообщения
void check_function(const char* function_name)
{
	// ОЧЕНЬ ВАЖНЫЙ МЕХАНИЗМ ПЕРЕДАЧИ АРГУМЕНТОВ В ФУНКЦИИ (без getchar() scanf() и т.п.) -----------------------------
	// И так нужно делать для каждой функции, которая должна принимать аргумент
	char name[15];
	int i = 0;
	for (i; i < 15 - 1; i++)
		{
			name[i] = function_name[i];
		}
	name[14] = '\0';
	// ----------------------------------------------------------------------------------------------------------------
		
	if(strcmp(name, "enter my name ") == 1) //true
	{
		read_user_name(function_name);
		return;
	}
	
	if(strcmp(function_name, "print my name") == 1)
	{
		print_user_name();
		return;
	}
	
	if(strcmp(function_name, "restart") == 1)
	{

		restart();	
		return;
	}
	
	if(strcmp(function_name, "exit") == 1)
	{
		// exit();
		exit_1();	
		return;
	}
	
	if(strcmp(function_name, "help") == 1)
	{
		print_help();	
		return;
	}
	
	if(strcmp(function_name, "clear") == 1)
	{
		clear_screen();	
		return;
	}
	
	if(strcmp(function_name, "logo") == 1)
	{
		print_logo_os();	
		return;
	}
	
	
	// если нет такой функции, то сообщить об этом
	kprint_newline();
	const char *str2 = "No such function.";
	kprint(str2);
	kprint_newline();
	return;

}


// В эти функции пока глубоко не вникал (но они выполняют простые операции общения (чтение/запись) с портами ввода/вывода, связанные с клавиатурой)
//------------ ports -----------------

uint8 inb (uint16 port) {

    uint8 result;

    __asm__("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}
void outb (uint16 port, uint8 data) {
    __asm__("out %%al, %%dx" : : "a" (data), "d" (port));
}

uint16 inw (uint16 port) {
    uint16 result;
    __asm__("in %%dx, %%ax" : "=a" (result) : "d" (port));
    return result;
}

void outw (uint16 port, uint16 data) {
    __asm__("out %%ax, %%dx" : : "a" (data), "d" (port));
}

// ----------- ports -----------------


// Инициализация экрана (возможно частично дублируется с idt_init(); и kb_init(); (НО ЭТО НЕ ТОЧНО))
void screen_init() {

    clear_screen();
	init_cursor(14, 15);
    set_cursor(0, 0);
}

void clear_screen() {

    uint8* video_memory = (uint8*)VIDEO_MEMORY_ADDRESS;
	uint16 i = 0;
	while(i < MAX_COLS * MAX_ROWS * 2) {
		video_memory[i] = SPACE_CHAR;
		video_memory[i+1] = BACKGROUND_COLOR_DEFALT; 		
		i = i + 2;
	}
	
	// Обнуление позиции РЕАЛЬНОГО курсора, связанного с видеопамятью
	current_loc = 0;
	set_current_position_visibale_cursor();
	// Обнуление количества введенных команд
	count_entered_string = 0;
	
}

// инициализация курсора
void init_cursor(uint8 cursor_start, uint8 cursor_end) {

	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);
	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

// установка курсора на экране (всопмогательная функция к функции "void set_current_position_visibale_cursor(void)")
void set_cursor(uint8 row, uint8 col) {

	uint16 pos = row * MAX_COLS + col;

	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8) ((pos >> 8) & 0xFF));

}

// Еще один способ реализации функции вывода на экран ("void kprint(const char*)" - лучше). Её пока тоже не удаляем, на всякий случай
/*
void print_string(uint8 row, uint8 col, const char* str) {

	uint8* video_memory = (uint8*)VIDEO_MEMORY_ADDRESS;
    uint32 i = (row * 80 + col) * 2;
    uint32 j = 0;
    while(str[j] != '\0') {
		video_memory[i] = str[j];
		video_memory[i+1] = BACKGROUND_COLOR_DEFALT;
		++j;
		i += 2;
	}

	set_cursor(row, col + j);
	row_current = row;
	col_current = col + j;
}
*/
void keyboard_handler_main(void)
{
	//const char *str1 = "we are in <keyboard_handler_main>";
	//kprint(str1);
	unsigned char status;
	char keycode;

	/* write EOI */
	write_port(0x20, 0x20);

	status = read_port(KEYBOARD_STATUS_PORT);
	/* Lowest bit of status will be set if buffer is not empty */
	if (status & 0x01) {
		keycode = read_port(KEYBOARD_DATA_PORT);
		if(keycode < 0)
			return;

		if(keycode == ENTER_KEY_CODE) {
			kprint_newline();
			const char *str1 = "You have entered the function_name: ";
			kprint(str1);
			//печатаем введенную строку
			kprint((const char *)msg);
			
			//проверяем, какая функция введена
			check_function((const char *)msg);
			
			clear_buffer_string();
			kprint_newline();
			kprint_newline();
			const char *str2 = "Enter a new function_name: ";
			kprint(str2);
			
			count_entered_string++;
			
			//проверка количества введенных строк (для своевременной очистки экрана)
			if (count_entered_string >= max_count_entered_string) 
			{
				clear_screen();
				//Выводим предложение ввести имя новой функции для выполнения
				const char *str1 = "Enter a new function_name: ";
				kprint(str1);
			}
			//exit(1); //не работает без библиотек
			return;
		}

		// проверка на превышение максимальной длины вводимого сообщения
		if (length_entered_sentense >=max_length -1) 
			{	
				kprint_newline();
				const char *str1 = "Length of your function_name more then MAX_LENGTH (80 symbols)";
				kprint(str1);
				kprint_newline();
				const char *str2 = "Your function_name before clearing: ";
				kprint(str2);
				//печатаем введенную строку (то, что было введено до того, как был превышен максимальный размер сообщения)
				kprint((const char *)msg);
				kprint_newline();
				clear_buffer_string();
				const char *str3 = "Your function_name has been cleared :(";
				kprint(str3);
				kprint_newline();
				kprint_newline();
				const char *str4 = "Enter a new function_name: ";
				kprint(str4);
				
				
				count_entered_string++;
			
				//проверка количества введенных строк (для своевременной очистки экрана)
				if (count_entered_string >= max_count_entered_string) 
				{
					clear_screen();
					//Выводим предложение ввести имя новой функции для выполнения
					const char *str1 = "Enter a new function_name: ";
					kprint(str1);
				}
				
				
			}
		//записываю слово в буффер посимвольно
		msg[length_entered_sentense++] = keyboard_map[(unsigned char) keycode];
		
		// вывод сообщения непосредственно в видеопамять (уже реализовано в функции "void kprint(const char*)")
		vidptr[current_loc++] = keyboard_map[(unsigned char) keycode];
		vidptr[current_loc++] = 0x07;
		
		set_current_position_visibale_cursor();
		
	}
}


// Это необходимо для правильной начальной реализации регистров в процессоре x86 (некоторые версии GRUB самостоятельно это делают, но лучше всегда иметь свой инициализатор GDT)

// ============= for GDT initialisation ======================

struct gdt_entry {
unsigned short limit_low;
unsigned short base_low;
unsigned char base_middle;
unsigned char access;
unsigned char granularity;
unsigned char base_high;
} __attribute__((packed));
struct gdt_ptr1 {
unsigned short limit;
unsigned int base;
} __attribute__((packed));
struct gdt_entry gdt[3];
struct gdt_ptr1 gp;
extern void gdt_flush();
void gdt_set_gate(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char gran) {
gdt[num].base_low = (base & 0xFFFF);
gdt[num].base_middle = (base >> 16) & 0xFF;
gdt[num].base_high = (base >> 24) & 0xFF;
gdt[num].limit_low = (limit & 0xFFFF);
gdt[num].granularity = ((limit >> 16) & 0x0F);
gdt[num].granularity |= (gran & 0xF0);
gdt[num].access = access;
}
void gdt_install() {
gp.limit = (sizeof(struct gdt_entry) * 3) - 1;
gp.base = &gdt;
gdt_set_gate(0, 0, 0, 0, 0);
gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
gdt_flush();
}


// ============= for GDT initialisation ======================



void kmain(void)
{
	screen_init();
	clear_screen();
	const char *str = "my first kernel with keyboard support";
	
	kprint(str);
	kprint_newline();
	kprint_newline();


	// в этих функция мы запускаем перехватываетля аппаратных прерываний от клавиатуры
	gdt_install();
	idt_init();
	kb_init();

	const char *str1 = "Enter a new function_name: ";
	kprint(str1);
	
	// Постоянно работает перехватыватель нажатий клавиш на клавиатуре
	while(1)
		asm volatile ("hlt");
}
