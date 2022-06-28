/******************************************************************************
 * File Name:   main.c
 *
 * Description: This is the source code for the USB FS bulk loop Example
 *              for ModusToolbox.
 *
 * Related Document: See README.md
 *
 *
*******************************************************************************
* Copyright 2021-2022, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/*******************************************************************************
 * Include header files
 ******************************************************************************/
#include "cy_pdl.h"
#include "cybsp.h"
#include "cy_usb_dev.h"
#include "cycfg_usbdev.h"

/*******************************************************************************
 * Macros
 ********************************************************************************/
#define USB_BUFFER_SIZE (64u)

#define IN_EP_NUM       (1u)
#define OUT_EP_NUM      (2u)


/*******************************************************************************
 * Function Prototypes
 ********************************************************************************/
static void usb_high_isr(void);
static void usb_medium_isr(void);
static void usb_low_isr(void);

/*******************************************************************************
 * Global Variables
 ********************************************************************************/

/* USB Interrupt Configuration */
const cy_stc_sysint_t usb_high_interrupt_cfg =
{
        .intrSrc = (IRQn_Type) usb_interrupt_hi_IRQn,
        .intrPriority = 0U,
};
const cy_stc_sysint_t usb_medium_interrupt_cfg =
{
        .intrSrc = (IRQn_Type) usb_interrupt_med_IRQn,
        .intrPriority = 1U,
};
const cy_stc_sysint_t usb_low_interrupt_cfg =
{
        .intrSrc = (IRQn_Type) usb_interrupt_lo_IRQn,
        .intrPriority = 2U,
};

/* USBDEV context variables */
cy_stc_usbfs_dev_drv_context_t usb_drvContext;
cy_stc_usb_dev_context_t usb_devContext;

/*******************************************************************************
 * Function Name: main
 ********************************************************************************
 * Summary:
 *  This is the main function. It initializes the USB Device block
 *  and enumerates as a vendor class device. It is constantly checking for data
 *  received from host and echos it back.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  int
 *
 *******************************************************************************/
int main(void)
{
    /* Stores result following specific operations */
    cy_rslt_t result;

    /* Array containing the data bytes received from host */
    uint8_t buffer[USB_BUFFER_SIZE];

    /* Stores the USBFS return code following specific operations */
    cy_en_usb_dev_status_t status;

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize the USB device */
    status = Cy_USB_Dev_Init(CYBSP_USB_HW, &CYBSP_USB_config, &usb_drvContext, &usb_devices[0], &usb_devConfig, &usb_devContext);
    if (status != CY_USB_DEV_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Initialize the USB interrupts */
    Cy_SysInt_Init(&usb_high_interrupt_cfg, &usb_high_isr);
    Cy_SysInt_Init(&usb_medium_interrupt_cfg, &usb_medium_isr);
    Cy_SysInt_Init(&usb_low_interrupt_cfg, &usb_low_isr);

    /* Enable the USB interrupts */
    NVIC_EnableIRQ(usb_high_interrupt_cfg.intrSrc);
    NVIC_EnableIRQ(usb_medium_interrupt_cfg.intrSrc);
    NVIC_EnableIRQ(usb_low_interrupt_cfg.intrSrc);

    /* Make device appear on the bus. This function call is blocking,
     * it waits until the device enumerates */
    Cy_USB_Dev_Connect(true, CY_USB_DEV_WAIT_FOREVER, &usb_devContext);

    uint16_t length;
    uint32_t actread_no = 0;

    for (;;)
    {
        Cy_USB_Dev_StartReadEp (OUT_EP_NUM, &usb_devContext);
        /* Check if data was received. */
        if (CY_USB_DEV_SUCCESS == Cy_USB_Dev_ReadEpBlocking(OUT_EP_NUM, buffer, USB_BUFFER_SIZE, &actread_no, CY_USB_DEV_WAIT_FOREVER, &usb_devContext))
        {
            /* Read number of received data bytes. */
            length = Cy_USB_Dev_GetEpNumToRead(OUT_EP_NUM, &usb_devContext);
            Cy_USB_Dev_WriteEpNonBlocking(IN_EP_NUM, buffer, length, &usb_devContext);
        }
    }
}

/***************************************************************************
 * Function Name: usb_high_isr
 ********************************************************************************
 * Summary:
 *  This function processes the high priority USB interrupts.
 *
 ***************************************************************************/
static void usb_high_isr(void)
{
    /* Call interrupt processing */
    Cy_USBFS_Dev_Drv_Interrupt(CYBSP_USB_HW, Cy_USBFS_Dev_Drv_GetInterruptCauseHi(CYBSP_USB_HW), &usb_drvContext);
}

/***************************************************************************
 * Function Name: usb_medium_isr
 ********************************************************************************
 * Summary:
 *  This function processes the medium priority USB interrupts.
 *
 ***************************************************************************/
static void usb_medium_isr(void)
{
    /* Call interrupt processing */
    Cy_USBFS_Dev_Drv_Interrupt(CYBSP_USB_HW, Cy_USBFS_Dev_Drv_GetInterruptCauseMed(CYBSP_USB_HW), &usb_drvContext);
}

/***************************************************************************
 * Function Name: usb_low_isr
 ********************************************************************************
 * Summary:
 *  This function processes the low priority USB interrupts.
 *
 **************************************************************************/
static void usb_low_isr(void)
{
    /* Call interrupt processing */
    Cy_USBFS_Dev_Drv_Interrupt(CYBSP_USB_HW, Cy_USBFS_Dev_Drv_GetInterruptCauseLo(CYBSP_USB_HW), &usb_drvContext);
}

/* [] END OF FILE */

