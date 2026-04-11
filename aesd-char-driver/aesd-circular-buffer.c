/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

void aesd_circular_buffer_increment_pointer(struct aesd_circular_buffer *buffer, int inout){
    //inout is 0 for in_offs, 1 for out_offs
    //This is completely arbitrary and has no semantic meaning
    
    uint8_t offs; // could use a pointer here, but this'll get optimized out anyway.

    if (inout == 0) {
        offs = buffer->in_offs;
    } else {
        offs = buffer->out_offs; 
    }

    // edge case is the last element. for 10 elements, it's index 9.
    if (offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - 1){
        offs = 0;
    } else {
        offs++;
    }

    if (inout == 0) {
        buffer->in_offs = offs;
    } else {
        buffer->out_offs = offs; 
    }
}



/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    size_t old_pos = 0;
    size_t new_pos = 0;
    uint8_t offs = buffer->out_offs; 
    //move one entry at a time
    int i;
    for (i=0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++){
        //entry at current offset's size
        new_pos = new_pos + buffer->entry[offs].size; //move the furthest location out to the end of the entry
        
        // if the char_offset is inside the current entry
        if (char_offset < new_pos) {
            old_pos = char_offset - old_pos;
            *entry_offset_byte_rtn = old_pos;     
    
            return &buffer->entry[offs];
        } else {
            // not in current entry, restart at next entry
            old_pos = new_pos;
            // edge case is the last element. for 10 elements, it's index 9.
            if (offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - 1){
                offs = 0;
            } else {
                offs++;
            }
        }
    }

    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    //write to entry
    buffer->entry[buffer->in_offs].buffptr = add_entry->buffptr;
    buffer->entry[buffer->in_offs].size = add_entry->size;

    //move out_offs
    if (buffer->full == true){
        aesd_circular_buffer_increment_pointer(buffer,1);
    }
    
    if (buffer->in_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED -1){
        buffer->full = true;
    }

    //move in_offs
    aesd_circular_buffer_increment_pointer(buffer,0);
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
    buffer->full = false;
    buffer->in_offs = 0;
    buffer->out_offs = 0;
}
