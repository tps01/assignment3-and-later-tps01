### Kernel oops from faulty:
> root@qemuarm64:~# echo "hello_world" > /dev/faulty
> [   88.418556] Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
> [   88.419002] Mem abort info:
> [   88.419052]   ESR = 0x0000000096000045
> [   88.419170]   EC = 0x25: DABT (current EL), IL = 32 bits
> [   88.419237]   SET = 0, FnV = 0
> [   88.419272]   EA = 0, S1PTW = 0
> [   88.419309]   FSC = 0x05: level 1 translation fault
> [   88.419519] Data abort info:
> [   88.419600]   ISV = 0, ISS = 0x00000045
> [   88.419727]   CM = 0, WnR = 1
> [   88.419961] user pgtable: 4k pages, 39-bit VAs, pgdp=0000000043761000
> [   88.420353] [0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
> [   88.425817] Internal error: Oops: 0000000096000045 [#1] PREEMPT SMP
> [   88.426215] Modules linked in: scull(O) hello(O) faulty(O)
> [   88.427647] CPU: 2 PID: 427 Comm: sh Tainted: G           O      5.15.199-yocto-standard #1
> [   88.427871] Hardware name: linux,dummy-virt (DT)
> [   88.428122] pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
> [   88.428369] pc : faulty_write+0x18/0x20 [faulty]
> [   88.428950] lr : vfs_write+0xf8/0x2a0
> [   88.429060] sp : ffffffc00972bd80
> [   88.429184] x29: ffffffc00972bd80 x28: ffffff8003e0d280 x27: 0000000000000000
> [   88.429529] x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
> [   88.430237] x23: 0000000000000000 x22: ffffffc00972bdc0 x21: 0000005558787560
> [   88.430774] x20: ffffff800375a700 x19: 000000000000000c x18: 0000000000000000
> [   88.431392] x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
> [   88.431566] x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
> [   88.431727] x11: 0000000000000000 x10: 0000000000000000 x9 : ffffffc008272268
> [   88.431875] x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
> [   88.431944] x5 : 0000000000000001 x4 : ffffffc000ba0000 x3 : ffffffc00972bdc0
> [   88.432005] x2 : 000000000000000c x1 : 0000000000000000 x0 : 0000000000000000
> [   88.432193] Call trace:
> [   88.432731]  faulty_write+0x18/0x20 [faulty]
> [   88.433289]  ksys_write+0x74/0x110
> [   88.433854]  __arm64_sys_write+0x24/0x30
> [   88.433956]  invoke_syscall+0x5c/0x130
> [   88.434444]  el0_svc_common.constprop.0+0x4c/0x100
> [   88.434564]  do_el0_svc+0x4c/0xc0
> [   88.434642]  el0_svc+0x28/0x80
> [   88.434954]  el0t_64_sync_handler+0xa4/0x130
> [   88.435093]  el0t_64_sync+0x1a0/0x1a4
> [   88.435364] Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
> [   88.437026] ---[ end trace fadadd5b50062d2d ]---
> /bin/start_getty: line 20:   427 Segmentation fault      ${setsid:-} ${getty} -L $1 $2 $3
> Poky (Yocto Project Reference Distro) 4.0.34 qemuarm64 /dev/ttyAMA0
> qemuarm64 login:

### Analysis:
Looking at the oops line:
> [   88.428369] pc : faulty_write+0x18/0x20 [faulty]
We can see that the error occurred at byte 18 in the faulty_write function.

The function source code is as follows:
> ssize_t faulty_write (struct file *filp, const char __user *buf, size_t count,
> 		loff_t *pos)
> {
> 	/* make a simple fault by dereferencing a NULL pointer */
> 	*(int *)0 = 0;
> 	return 0;
> }

The comment in the function makes this clear, but regardless we would be able to see that this is dereferencing a NULL pointer.