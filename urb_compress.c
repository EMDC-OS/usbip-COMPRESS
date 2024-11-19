#include <linux/file.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/crypto.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/scatterlist.h>
#include <linux/export.h> 
#include "usbip_common.h"


//LZO_RLE
//compression
void urb_cprs(struct urb *urb){
    int ret;
    //compression for iso packets
    if (usb_pipetype(urb->pipe) == PIPE_ISOCHRONOUS){
        ret = urb_cprs_iso(urb);
        if (ret < 0) {
            pr_err("URB compression failed with error: %d\n", ret);
        } else {
            pr_info("URB compression succeeded.\n");
        }
    }

}
EXPORT_SYMBOL(urb_cprs);


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
   //using linux kernel crypto model
    
    struct crypto_comp *tfm;
    void *workspace = NULL;
    int i, ret = 0;

    if (!urb || !urb->transfer_buffer)
        return -EINVAL;

    // Allocate LZO-RLE compression transform
    tfm = crypto_alloc_comp("lzo-rle", 0, 0);
    if (IS_ERR(tfm)) {
        pr_err("Failed to allocate LZO-RLE transform\n");
        return PTR_ERR(tfm);
    }

    /* Allocate workspace for compression
    workspace = kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (!workspace) {
        ret = -ENOMEM;
        goto out_free_tfm;
    }*/

    // Process each ISO packet
    for (i = 0; i < urb->number_of_packets; i++) {
        struct usb_iso_packet_descriptor *desc = &urb->iso_frame_desc[i];
        unsigned int offset = desc->offset;
        unsigned int length = desc->actual_length; // Original length
        unsigned int compressed_len = 0;
        void *src, *dest;

        if (length == 0)
            continue;
        //acutual_length < offset
        //offset is determined by the HW
        //before transmit, it will have padding
        src = urb->transfer_buffer + offset; 
        dest = kmalloc(length, GFP_KERNEL);  // Allocate temporary buffer
        if (!dest) {
            ret = -ENOMEM;
            return ret;
        }

        // Compress the data
        ret = crypto_comp_compress(tfm, src, length, dest, &compressed_len);
        if (ret < 0) {
            pr_err("Compression failed for packet %d\n", i);
            kfree(dest);
            continue; // Skip to the next packet
        }

        if (compressed_len > length) {
            pr_err("Compressed data exceeds original size, skipping packet %d\n", i);
            kfree(dest);
            continue;
        }

        // Copy compressed data back to the urb transfer buffer
        //because of padding it will not overbound?

        memcpy(src, dest, compressed_len);
        memset(src + compressed_len, 0, length - compressed_len);
        desc->actual_length = compressed_len; // Update actual length

        pr_info("Compressed packet %d: original=%u, compressed=%u\n",
                i, length, compressed_len);

        kfree(dest);

    }

/*out_free_workspace:
    kfree(workspace);
out_free_tfm:
    crypto_free_comp(tfm);
    return ret;*/
    crypto_free_comp(tfm);
    return ret;
}

//decomrpession

void urb_dcprs(struct urb *urb){
    int ret;
    //decompression for iso packets
    if (usb_pipeisoc(urb->pipe)){
        ret = urb_dcprs_iso(urb);
        if (ret < 0) {
            pr_err("URB compression failed with error: %d\n", ret);
        } else {
            pr_info("URB compression succeeded.\n");
        }
    }
}
EXPORT_SYMBOL(urb_dcprs);



//decompression after adding the padding back to the iso packets
int urb_dcprs_iso(struct urb *urb){
    struct crypto_comp *tfm;
    int i, ret = 0;

    tfm = crypto_alloc_comp("lzo-rle", 0, 0);
    if (IS_ERR(tfm)) {
        pr_err("Failed to allocate LZO-RLE transform\n");
        return PTR_ERR(tfm);
    }

    for (i = 0; i < urb->number_of_packets; i++) {
        struct usb_iso_packet_descriptor *desc = &urb->iso_frame_desc[i];
        unsigned int offset = desc->offset;
        unsigned int length = desc->actual_length;
        unsigned int decompressed_len = PAGE_SIZE; // Decompressed size (estimate)
        void *src, *dest;

        if (length == 0)
            continue;

        src = urb->transfer_buffer + offset;
        dest = kmalloc(PAGE_SIZE, GFP_KERNEL);
        if (!dest) {
            ret = -ENOMEM;//decompression after adding the padding back to the iso packets
int urb_dcprs_iso(struct urb *urb){
    struct crypto_comp *tfm;
    int i, ret = 0;

    tfm = crypto_alloc_comp("lzo-rle", 0, 0);
    if (IS_ERR(tfm)) {
        pr_err("Failed to allocate LZO-RLE transform\n");
        return PTR_ERR(tfm);
    }

    for (i = 0; i < urb->number_of_packets; i++) {
        struct usb_iso_packet_descriptor *desc = &urb->iso_frame_desc[i];
        unsigned int offset = desc->offset;
        unsigned int length = desc->actual_length;
        unsigned int decompressed_len = desc->length; // Decompressed size (estimate)
        void *src, *dest;

        if (length == 0)
            continue;

        src = urb->transfer_buffer + offset;
        dest = kmalloc(PAGE_SIZE, GFP_KERNEL);
        if (!dest) {
            ret = -ENOMEM;
            break;
        }

        ret = crypto_comp_decompress(tfm, src, length, dest, &decompressed_len);
        if (ret < 0) {
            pr_err("Decompression failed for packet %d\n", i);
            kfree(dest);
            continue;
        }

        if (decompressed_len < length || decompressed_len > desc->length) {
            pr_err("Compressed data exceeds original size, skipping packet %d\n", i);
            kfree(dest);
            continue;
        }

        memcpy(src, dest, decompressed_len);
        desc->actual_length = decompressed_len;

        kfree(dest);
    }

    crypto_free_comp(tfm);
    return ret;

}
            break;
        }

        ret = crypto_comp_decompress(tfm, src, length, dest, &decompressed_len);
        if (ret < 0) {
            pr_err("Decompression failed for packet %d\n", i);
            kfree(dest);
            continue;
        }

        if (decompressed_len < length || decompressed_len > offset) {
            pr_err("Compressed data exceeds original size, skipping packet %d\n", i);
            kfree(dest);
            continue;
        }

        memcpy(src, dest, decompressed_len);
        desc->actual_length = decompressed_len;

        kfree(dest);
    }

    crypto_free_comp(tfm);
    return ret;

}