#include <linux/file.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include "usbip_common.h"


//LZO_RLE
//compression
void urb_cprs(struct urb *urb){
    //compression for iso packets
    if (usb_pipetype(urb->pipe) == PIPE_ISOCHRONOUS){
        urb_cprs_iso(urb);
    }

}


int urb_cprs_iso(struct urb *urb){
    //if compression failed, use original data
    //compression flag <-- where to add????

    /*
    iso packet:
    struct usb_iso_packet_descriptor {
	unsigned int offset;
	unsigned int length;
	unsigned int actual_length;
	int status;
    */
   //add compression flag on the status





}

//decomrpession
void urb_dcprs(struct urb *urb){
    //decompression for iso packets
    if (usb_pipeisoc(urb->pipe)){
        urb_dcprs_iso(urb);
    }
}

int urb_dcprs_iso(struct urb *urb){

}