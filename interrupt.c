#include "interrupt.h"

/*

*/

int register_irq(unsigned long irq,
		void * arg,
		void (*handler)(unsigned long nr, unsigned long parameter, struct pt_regs * regs),
		unsigned long parameter,
		hw_int_controller * controller,
		char * irq_name)
{	
	irq_desc_T * p = &interrupt_desc[irq - 32];
	
	p->controller = controller;
	p->irq_name = irq_name;
	p->parameter = parameter;
	p->flags = 0;
	p->handler = handler;

	p->controller->install(irq,arg);
	p->controller->enable(irq);
	
	return 1;
}

/*

*/

int unregister_irq(unsigned long irq)
{
	irq_desc_T * p = &interrupt_desc[irq - 32];

	p->controller->disable(irq);
	p->controller->uninstall(irq);

	p->controller = NULL;
	p->irq_name = NULL;
	p->parameter = NULL;
	p->flags = 0;
	p->handler = NULL;

	return 1; 
}

int register_IPI(unsigned long irq,
		void * arg,
		void (*handler)(unsigned long nr, unsigned long parameter, struct pt_regs * regs),
		unsigned long parameter,
		hw_int_controller * controller,
		char * irq_name)
{	
	irq_desc_T * p = &SMP_IPI_desc[irq - 200];
	
	p->controller = NULL;
	p->irq_name = irq_name;
	p->parameter = parameter;
	p->flags = 0;
	p->handler = handler;
	
	return 1;
}

/*

*/

int unregister_IPI(unsigned long irq)
{
	irq_desc_T * p = &SMP_IPI_desc[irq - 200];

	p->controller = NULL;
	p->irq_name = NULL;
	p->parameter = NULL;
	p->flags = 0;
	p->handler = NULL;

	return 1; 
}
