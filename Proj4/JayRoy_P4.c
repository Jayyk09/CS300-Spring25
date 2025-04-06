/**
 * Virtual Memory Manager - Part 1
 * 
 * This program simulates a virtual memory system with:
 * - 16-bit virtual address space (65,536 bytes)
 * - 8-bit page number and 8-bit offset
 * - 256 pages of 256 bytes each
 * - 256 frames in physical memory
 * - 16-entry TLB with FIFO replacement
 * - On-demand paging with backing store
 * 
 * The program translates logical addresses to physical addresses,
 * handles page faults, and manages the TLB.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Constants
#define PAGE_SIZE 256          // Size of each page/frame (in bytes)
#define PAGE_TABLE_SIZE 256    // Number of entries in page table (pages)
#define TLB_SIZE 16            // Number of entries in TLB
#define FRAME_COUNT 256        // Number of frames in physical memory
#define MEMORY_SIZE (FRAME_COUNT * PAGE_SIZE) // Total size of physical memory
#define ADDRESS_MASK 0xFFFF    // Mask for extracting 16 least significant bits

// TLB entry structure
typedef struct {
    int page_number;
    int frame_number;
    bool valid;
} TLB_Entry;

// Global variables
TLB_Entry tlb[TLB_SIZE];                      // TLB with 16 entries
int page_table[PAGE_TABLE_SIZE];              // Page table with 256 entries
signed char physical_memory[MEMORY_SIZE];     // Physical memory (65,536 bytes)
FILE *backing_store;                          // File pointer for backing store
int page_faults = 0;                          // Counter for page faults
int tlb_hits = 0;                             // Counter for TLB hits
int total_addresses = 0;                      // Counter for total addresses processed
int free_frame = 0;                           // Next available frame index
int tlb_index = 0;                            // Current index in TLB (for FIFO)

int main(int argc, char *argv[]) {
    // Check if correct number of arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s addresses_file\n", argv[0]);
        return -1;
    }

    // Open the addresses file
    FILE *addresses_file = fopen(argv[1], "r");
    if (addresses_file == NULL) {
        fprintf(stderr, "Error: Could not open file %s\n", argv[1]);
        return -1;
    }

    // Open the backing store
    backing_store = fopen("BACKING_STORE.bin", "rb");
    if (backing_store == NULL) {
        fprintf(stderr, "Error: Could not open BACKING_STORE.bin\n");
        fclose(addresses_file);
        return -1;
    }

    // Initialize page table - all entries initially invalid (-1)
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        page_table[i] = -1;
    }

    // Initialize TLB - all entries initially invalid
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].valid = false;
    }

    // Process addresses from the file
    int logical_address;
    while (fscanf(addresses_file, "%d", &logical_address) != EOF) {
        total_addresses++;
        
        // Mask the logical address to get only the 16 least significant bits
        logical_address = logical_address & ADDRESS_MASK;
        
        // Extract page number and offset from logical address
        int page_number = logical_address >> 8;     // High 8 bits
        int offset = logical_address & 0xFF;        // Low 8 bits
        
        int frame_number = -1;
        bool tlb_hit = false;
        
        // Check TLB for page number
        for (int i = 0; i < TLB_SIZE; i++) {
            if (tlb[i].valid && tlb[i].page_number == page_number) {
                frame_number = tlb[i].frame_number;
                tlb_hit = true;
                tlb_hits++;
                break;
            }
        }
        
        // If not in TLB, check page table
        if (!tlb_hit) {
            // Check if page is in page table
            if (page_table[page_number] != -1) {
                frame_number = page_table[page_number];
            } else {
                // Page fault - load from backing store
                page_faults++;
                
                // Seek to page position in backing store
                fseek(backing_store, page_number * PAGE_SIZE, SEEK_SET);
                
                // Read page into memory
                signed char buffer[PAGE_SIZE];
                fread(buffer, sizeof(signed char), PAGE_SIZE, backing_store);
                
                // Allocate a frame
                frame_number = free_frame;
                free_frame++;
                
                // Copy page into physical memory
                for (int i = 0; i < PAGE_SIZE; i++) {
                    physical_memory[frame_number * PAGE_SIZE + i] = buffer[i];
                }
                
                // Update page table
                page_table[page_number] = frame_number;
            }
            
            // Update TLB (FIFO replacement)
            tlb[tlb_index].page_number = page_number;
            tlb[tlb_index].frame_number = frame_number;
            tlb[tlb_index].valid = true;
            tlb_index = (tlb_index + 1) % TLB_SIZE;
        }
        
        // Calculate physical address
        int physical_address = frame_number * PAGE_SIZE + offset;
        
        // Get byte value from physical memory
        signed char value = physical_memory[physical_address];
        
        // Output the address translation
        printf("Virtual address: %d Physical address: %d Value: %d\n", 
               logical_address, physical_address, value);
    }
    
    // Print statistics
    printf("\nNumber of Translated Addresses = %d\n", total_addresses);
    printf("Page Faults = %d\n", page_faults);
    printf("Page Fault Rate = %.3f\n", (double)page_faults / total_addresses);
    printf("TLB Hits = %d\n", tlb_hits);
    printf("TLB Hit Rate = %.3f\n", (double)tlb_hits / total_addresses);
    
    // Close files
    fclose(addresses_file);
    fclose(backing_store);
    
    return 0;
}