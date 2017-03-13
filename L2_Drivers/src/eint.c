#ifdef __cplusplus
extern "C" {
#endif

#include "eint.h"
#include <stdlib.h>
#include "core_cm3.h"
#include "LPC17xx.h"
#include "c_list.h"
#include "printf_lib.h"

/*
 * Must write your own implementation of eint.h
 */

typedef struct {
    eint_intr_t type;
    void_func_t callback;
    uint8_t pin_num;
} gpio_callback_t;

c_list_ptr p0_rising_callbacks = NULL;
c_list_ptr p0_falling_callbacks = NULL;
c_list_ptr p2_rising_callbacks = NULL;
c_list_ptr p2_falling_callbacks = NULL;

/* Utility functions */
c_list_ptr * select_callback_list(uint8_t port_num, eint_intr_t type);
uint32_t * select_interrupt_reg(uint8_t port_num, eint_intr_t type);
gpio_callback_t * create_callback_entry(uint8_t pin_num, eint_intr_t type, void_func_t * func);

static bool callback_iterate(void * elm, void *statusrising, void *statusfalling, void *arg3)
{
    gpio_callback_t* callback_entry = (gpio_callback_t*) elm;
    if (callback_entry->type == eint_rising_edge)
    {
        if ((*(uint32_t*)statusrising) & (1 << callback_entry->pin_num))
        {
            (*callback_entry->callback)();
        }
    }
    else
    {
        if ((*(uint32_t*)statusfalling) & ( 1 << callback_entry->pin_num))
        {
            (*callback_entry->callback)();
        }
    }
    return true;
}

void EINT3_IRQHandler(void)
{
    /* Use c_list_for_each_elm to trigger callbacks */
    /* List needs to contain more than just the callback.. need to check the pin # too */
    uint8_t intstatus = LPC_GPIOINT->IntStatus;
    const uint8_t P0_PENDING = (1 << 0);
    const uint8_t P2_PENDING = (1 << 2);
    uint32_t p0_falling_status = LPC_GPIOINT->IO0IntStatF;
    uint32_t p0_rising_status = LPC_GPIOINT->IO0IntStatR;
    uint32_t p2_falling_status = LPC_GPIOINT->IO2IntStatF;
    uint32_t p2_rising_status = LPC_GPIOINT->IO2IntStatR;
    if (intstatus & P0_PENDING)
    {
        /* Loop through P0 callbacks for falling and rising */
        while (!c_list_for_each_elm(p0_rising_callbacks, callback_iterate, &p0_rising_status, &p0_falling_status, NULL)) { ; }
        while (!c_list_for_each_elm(p0_falling_callbacks, callback_iterate, &p0_rising_status, &p0_falling_status, NULL)) { ; }
    }

    if (intstatus & P2_PENDING)
    {
        /* Loop through P2 callbacks for falling and rising */
        while (!c_list_for_each_elm(p2_rising_callbacks, callback_iterate, &p2_rising_status, &p2_falling_status, NULL)) { ; }
        while (!c_list_for_each_elm(p2_falling_callbacks, callback_iterate, &p2_rising_status, &p2_falling_status, NULL)) { ; }
    }

    /* Assume all interrupts are serviced */
    LPC_GPIOINT->IO0IntClr = ~(0);
    LPC_GPIOINT->IO2IntClr = ~(0);
}

void eint3_enable_port0(uint8_t pin_num, eint_intr_t type, void_func_t func)
{
    if (pin_num >= 32)
    {
        return;
    }

    const uint8_t port_num = 0;
    uint32_t * intreg = select_interrupt_reg(port_num, type);
    c_list_ptr * callback_list = select_callback_list(port_num, type);
    gpio_callback_t * entry = create_callback_entry(pin_num, type, &func);

    c_list_insert_elm_end(*callback_list, entry);
    *intreg |= (1 << pin_num);
    NVIC_EnableIRQ(EINT3_IRQn);
}

void eint3_enable_port2(uint8_t pin_num, eint_intr_t type, void_func_t func)
{
    if (pin_num >= 32)
    {
        return;
    }

    const uint8_t port_num = 2;
    uint32_t * intreg = select_interrupt_reg(port_num, type);
    c_list_ptr * callback_list = select_callback_list(port_num, type);
    gpio_callback_t * entry = create_callback_entry(pin_num, type, &func);

    c_list_insert_elm_end(*callback_list, entry);
    *intreg |= (1 << pin_num);
    NVIC_EnableIRQ(EINT3_IRQn);
}


c_list_ptr * select_callback_list(uint8_t port_num, eint_intr_t type)
{
    c_list_ptr * cb_list = NULL;
    switch (port_num)
    {
        case 0:
            if (type == eint_rising_edge)
            {
                cb_list = &p0_rising_callbacks;
            }
            else
            {
                cb_list = &p0_falling_callbacks;
            }
            break;
        case 2:
            if (type == eint_rising_edge)
            {
                cb_list = &p2_rising_callbacks;
            }
            else
            {
                cb_list = &p2_falling_callbacks;
            }
            break;
        default:
            return NULL;
    }

    if (*cb_list == NULL)
    {
        *cb_list = c_list_create();
    }

    return cb_list;
}

uint32_t * select_interrupt_reg(uint8_t port_num, eint_intr_t type)
{
    if (type == eint_rising_edge)
    {
        if (port_num == 0)
        {
            return (uint32_t *)&LPC_GPIOINT->IO0IntEnR;
        }
        else if (port_num == 2)
        {
            return (uint32_t *)&LPC_GPIOINT->IO2IntEnR;
        }
    }
    else
    {
        if (port_num == 0)
        {
            return (uint32_t *)&LPC_GPIOINT->IO0IntEnF;
        }
        else if (port_num == 2)
        {
            return (uint32_t *)&LPC_GPIOINT->IO2IntEnF;
        }
    }

    return NULL;
}

gpio_callback_t * create_callback_entry(uint8_t pin_num, eint_intr_t type, void_func_t * func)
{
    /* Must keep in memory */
    gpio_callback_t * entry = malloc(sizeof(gpio_callback_t));
    if (entry == 0)
    {
        return NULL;
    }

    entry->callback = *func;
    entry->type = type;
    entry->pin_num = pin_num;
    return entry;
}


#ifdef __cplusplus
}
#endif
