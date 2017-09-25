#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/semaphore.h>




// Keyboard Ports
#define KBD_DATA                0x60   // I/O port for keyboard data
#define KBD_STATUS              0x64   // I/O port for keyboard status (read)

// Keyboard Commands
#define KBD_CMD_SET_LEDS        0xED   // Set keyboard leds

// Keyboard Replies
#define KBD_REPLY_ACK           0xFA    // Command ACK

// Keyboard LEDS Numbers
#define LED_NUM_LOCK            2
#define LED_SCROLL_LOCK         1
#define LED_CAPS_LOCK           4
#define LED_NUM_LOCK_OFF        5
#define LED_SCROLL_LOCK_OFF     6
#define LED_CAPS_LOCK_OFF       3

// Keyboard Status Register Bits
#define KBD_STAT_OBF            0x01    // Keyboard output buffer full
#define KBD_STAT_IBF            0x02    // Keyboard input buffer full

// Semaphores
#define COUNTER_VALUE           1



// Keyboard General status
unsigned char led_status = 0;
// Files names in kernel module
static int caps;
static int num;
static int scroll;

static struct semaphore sema;

// Read data from port 0x64
unsigned char kbd_read_status(void)
{
    unsigned char data = inb(KBD_STATUS);
    return data;
}

// Read data from port 0x60
unsigned char kbd_read_data(void)
{
    unsigned char status;
    status = kbd_read_status();
    while (((status) & KBD_STAT_OBF) != 1)
    {
        status = kbd_read_status();
    }
    return inb(KBD_DATA);

}

// Write data to port 0x60
void kbd_write_data(unsigned char data)
{
    unsigned char status;
    status =kbd_read_status();
    while (((status) & KBD_STAT_IBF) != 0)
    {
        status = kbd_read_status();
    }
    outb(data,KBD_DATA);
}

// Semaphores function to achieve mutual exclusion
void init(void)
{
    sema_init(&sema, COUNTER_VALUE);
}

// Update the led by writing the new state
int update_leds(unsigned char led_status_word)
{
    printk(" process[%d] 3 start update function\n",current->pid);
    // Disabling the interruption of keyboard
    disable_irq(1);
    printk(" process[%d] 4 Disable irq\n",current->pid);
    // send _Set LEDs_ command
    kbd_write_data(KBD_CMD_SET_LEDS);
    printk(" process[%d] 5 sent set leds \n",current->pid);
    msleep(500);
    // wait for ACK
    if (kbd_read_data() != KBD_REPLY_ACK )
    {
        enable_irq(1);
        return -1;
    }
    printk(" process[%d] 6 wait for first ACK\n",current->pid);
    // now send LED states
    kbd_write_data(led_status_word);
    printk(" process[%d] 7 send led states\n",current->pid);
    // wait for ACK
    if (kbd_read_data() != KBD_REPLY_ACK )
    {
        enable_irq(1);
        return -1;
    }
    printk(" process[%d] 8 wait for second ACK\n",current->pid);
    // success and enable the interruption
    enable_irq(1);
    printk(" process[%d] 9 success \n",current->pid);
    printk(" process[%d] 10 end update function\n",current->pid);
    return 0;
}


// Get the current state of the led num/caps/scroll by index
int get_led_state(int led )
{
    switch(led)
    {
    case 0:
        return scroll;
        break;
    case 1:
        return num;
        break;
    case 2 :
        return caps;
        break;
    }
    return -1;
}


void set_led_state(int led, int state)
{
    // Set the new state of the led
    printk(" process[%d] 1 start set function\n",current->pid);
    if(state)
    {
        //If state is one do bitwise or the old led_state to keep the values of the other leds
        switch(led)
        {
        case 0:
            led_status = led_status | LED_SCROLL_LOCK;
            break;
        case 1:
            led_status = led_status | LED_NUM_LOCK;
            break;
        case 2:
            led_status = led_status | LED_CAPS_LOCK;
            break;
        }
    }
    else
    {
        // If state is zero do bitwise and with
        switch(led)
        {
        case 0:
            led_status =led_status & LED_SCROLL_LOCK_OFF;
            break;
        case 1:
            led_status  = led_status & LED_NUM_LOCK_OFF;
            break;
        case 2:
            led_status= led_status & LED_CAPS_LOCK_OFF;
            break;
        }
    }
    printk(" process[%d] 2 will update in set\n",current->pid);
    update_leds(led_status);
    printk(" process[%d] 11 finish update in set\n",current->pid);
}

// Show the state of the Num lock button
static ssize_t num_show(struct kobject *kobj,struct kobj_attribute *attr,char *buf)
{
    up(&sema);
    int state = get_led_state(1);
    down(&sema);
    return sprintf(buf,"%d\n",state);
}

// Store the new state of the Num lock into the file
static ssize_t num_store(struct kobject *kobj, struct kobj_attribute *attr,
                         const char *buf, size_t count)
{
    int ret;
    up(&sema);
    ret = kstrtoint(buf, 10, &num);
    if (ret < 0)
    {
        return ret;
    }
    set_led_state(1,num);
    down(&sema);
    return count;
}

// Create the attribute of the kernel object (Num lock)
static struct kobj_attribute num_attribute =
    __ATTR(num, 0664, num_show, num_store);

// Generalization function of the rest button (SCROLL_LOCK/CAPS_LOCK) to get the current state
static ssize_t child_show(struct kobject *kobj, struct kobj_attribute *attr,
                          char *buf)
{
    int var;
    up(&sema);
    if (strcmp(attr->attr.name, "caps") == 0)
    {
        var = get_led_state(2);
    }
    else
    {
        var = get_led_state(0);
    }
    down(&sema);
    return sprintf(buf, "%d\n", var);
}

// Generalization function for the rest button(SCROLL_LOCK/CAPS_LOCK) to set the new state
static ssize_t child_store(struct kobject *kobj, struct kobj_attribute *attr,
                           const char *buf, size_t count)
{
    int ret,var;
    up(&sema);
    ret = kstrtoint(buf, 10, &var);
    if (ret < 0)
        return ret;

    if (strcmp(attr->attr.name, "caps") == 0)
    {
        caps = var;
        set_led_state(2,var);

    }
    else if(strcmp(attr->attr.name, "scroll") == 0)
    {
        scroll = var;
        set_led_state(0,var);

    }
    down(&sema);
    return count;
}

// Create the attribute of (SCROLL_LOCK/CAPS_LOCK)
static struct kobj_attribute caps_attribute =
    __ATTR(caps, 0664, child_show, child_store);
static struct kobj_attribute scroll_attribute =
    __ATTR(scroll, 0664, child_show, child_store);

static struct attribute *attrs[] =
{
    &num_attribute.attr,
    &caps_attribute.attr,
    &scroll_attribute.attr,
    NULL,
};

static struct attribute_group attr_group =
{

    .attrs = attrs,

};

static struct kobject *example_kobj;

static int __init example_init(void)
{
    int retval;
    example_kobj = kobject_create_and_add("kobject_safaa", kernel_kobj);
    if (!example_kobj)
        return -ENOMEM;
    printk("Create k_object");
    /* Create the files associated with this kobject */
    retval = sysfs_create_group(example_kobj, &attr_group);
    if (retval)
        kobject_put(example_kobj);
    init();
    return retval;
}
static void __exit example_exit(void)
{
    kobject_put(example_kobj);
}

module_init(example_init);

module_exit(example_exit);


MODULE_LICENSE("GPL");

