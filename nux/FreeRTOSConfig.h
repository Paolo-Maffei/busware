#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

/* The maximum number of messages that can be waiting for process at any one
time. */
#define UART_QUEUE_SIZE						( 128 )


#define configUSE_PREEMPTION			1
#define configUSE_IDLE_HOOK				0
#define configUSE_TICK_HOOK				0
#define configCPU_CLOCK_HZ				( ( unsigned long ) 50000000 )
#define configTICK_RATE_HZ				( ( portTickType ) 1000 )
#define configMINIMAL_STACK_SIZE		( ( unsigned short ) 80 )
#define configTOTAL_HEAP_SIZE			( ( size_t ) ( 10000 ) )
#define configMAX_TASK_NAME_LEN			( 12 )
#define configUSE_TRACE_FACILITY		0
#define configUSE_16_BIT_TICKS			0
#define configIDLE_SHOULD_YIELD			0
#define configUSE_CO_ROUTINES 			0
#define configUSE_MUTEXES				1
#define configCHECK_FOR_STACK_OVERFLOW	2
#define configUSE_RECURSIVE_MUTEXES		1
#define configQUEUE_REGISTRY_SIZE		10
#define configGENERATE_RUN_TIME_STATS	0
#define configUSE_COUNTING_SEMAPHORES   1

#define configMAX_PRIORITIES		( ( unsigned portBASE_TYPE ) 5 )
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet			1
#define INCLUDE_uxTaskPriorityGet			1
#define INCLUDE_vTaskDelete					1
#define INCLUDE_vTaskCleanUpResources		0
#define INCLUDE_vTaskSuspend				1
#define INCLUDE_vTaskDelayUntil				1
#define INCLUDE_vTaskDelay					1
#define INCLUDE_uxTaskGetStackHighWaterMark	1



#define configKERNEL_INTERRUPT_PRIORITY 		( ( unsigned char ) 7 << ( unsigned char ) 5 )	/* Priority 7, or 255 as only the top three bits are implemented.  This is the lowest priority. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	( ( unsigned char ) 5 << ( unsigned char ) 5 )  /* Priority 5, or 160 as only the top three bits are implemented. */
#define SET_SYSCALL_INTERRUPT_PRIORITY(X) (((X) << 5)&0xE0)


#endif /* FREERTOS_CONFIG_H */
