/**
 * Project 4 - Part 2: Virtual Memory Manager
 * 
 * Name: Jay Roy
 * Date: 04/06/2025
 * Usage: ./program_name addresses_file [frame_count]
 * CWID: 12342760
 * 
 * This program extends Part 1 by:
 * - Supporting variable-sized physical memory (fewer frames than virtual pages)
 * - Implementing LRU page replacement when physical memory is full
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

// Constants
#define PAGE_SIZE 256          // Size of each page/frame (in bytes)
#define PAGE_TABLE_SIZE 256    // Number of entries in page table (pages)
#define TLB_SIZE 16            // Number of entries in TLB
#define DEFAULT_FRAME_COUNT 128 // Default number of frames if not specified
#define MAX_FRAMES 256         // Maximum possible frames
#define ADDRESS_MASK 0xFFFF    // Mask for extracting 16 least significant bits

// TLB entry structure
typedef struct {
    int page_number;
    int frame_number;
    bool valid;
} TLB_Entry;

// Page table entry with validity flag
typedef struct {
    int frame_number;
    bool valid;
} PageTableEntry;

// Global variables
TLB_Entry tlb[TLB_SIZE];                      // TLB with 16 entries
PageTableEntry page_table[PAGE_TABLE_SIZE];   // Page table with 256 entries
signed char *physical_memory;                 // Physical memory (dynamically allocated)
int *frame_last_used;                         // Array to track when each frame was last used
int frame_count;                              // Number of frames in physical memory
FILE *backing_store;                          // File pointer for backing store
int page_faults = 0;                          // Counter for page faults
int tlb_hits = 0;                             // Counter for TLB hits
int total_addresses = 0;                      // Counter for total addresses processed
int free_frame = 0;                           // Next available frame index (up to frame_count)
int tlb_index = 0;                            // Current index in TLB (for FIFO)
int current_time = 0;                         // Clock for LRU algorithm

// Function to find the least recently used frame
int find_lru_frame() {
    int lru_frame = 0;
    int min_time = INT_MAX;
    
    for (int i = 0; i < frame_count; i++) {
        if (frame_last_used[i] < min_time) {
            min_time = frame_last_used[i];
            lru_frame = i;
        }
    }
    
    return lru_frame;
}

// Function to find which page is using a specific frame
int find_page_using_frame(int frame) {
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        if (page_table[i].valid && page_table[i].frame_number == frame) {
            return i;
        }
    }
    return -1; // Not found
}

int main(int argc, char *argv[]) {
    // Check if correct number of arguments
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s addresses_file [frame_count]\n", argv[0]);
        return -1;
    }

    // Determine the number of frames in physical memory
    if (argc == 3) {
        frame_count = atoi(argv[2]);
        if (frame_count <= 0 || frame_count > MAX_FRAMES) {
            fprintf(stderr, "Error: Frame count must be between 1 and %d\n", MAX_FRAMES);
            return -1;
        }
    } else {
        frame_count = DEFAULT_FRAME_COUNT;
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

    // Allocate physical memory based on frame count
    physical_memory = (signed char *)malloc(frame_count * PAGE_SIZE);
    if (physical_memory == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(addresses_file);
        fclose(backing_store);
        return -1;
    }

    // Allocate and initialize frame usage tracking
    frame_last_used = (int *)malloc(frame_count * sizeof(int));
    if (frame_last_used == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        free(physical_memory);
        fclose(addresses_file);
        fclose(backing_store);
        return -1;
    }
    
    for (int i = 0; i < frame_count; i++) {
        frame_last_used[i] = -1; // Initialize to -1 (never used)
    }

    // Initialize page table - all entries initially invalid
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        page_table[i].valid = false;
    }

    // Initialize TLB - all entries initially invalid
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].valid = false;
    }

    printf("# of frames: %d \n", frame_count);

    // Process addresses from the file
    int logical_address;
    while (fscanf(addresses_file, "%d", &logical_address) != EOF) {
        total_addresses++;
        current_time++; // Increment time for LRU
        
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
                // Update the last used time for this frame
                frame_last_used[frame_number] = current_time;
                break;
            }
        }
        
        // If not in TLB, check page table
        if (!tlb_hit) {
            // Check if page is in page table
            if (page_table[page_number].valid) {
                frame_number = page_table[page_number].frame_number;
                // Update the last used time for this frame
                frame_last_used[frame_number] = current_time;
            } else {
                // Page fault - load from backing store
                page_faults++;
                
                // Seek to page position in backing store
                fseek(backing_store, page_number * PAGE_SIZE, SEEK_SET);
                
                // Read page into a temporary buffer
                signed char buffer[PAGE_SIZE];
                fread(buffer, sizeof(signed char), PAGE_SIZE, backing_store);
                
                // Allocate a frame - either a free one or replace using LRU
                if (free_frame < frame_count) {
                    // We still have free frames available
                    frame_number = free_frame;
                    free_frame++;
                } else {
                    // No free frames - use LRU replacement
                    frame_number = find_lru_frame();
                    
                    // Find which page is using this frame
                    int old_page = find_page_using_frame(frame_number);
                    if (old_page != -1) {
                        // Invalidate the page table entry
                        page_table[old_page].valid = false;
                        
                        // Also invalidate any TLB entries referencing this page
                        for (int i = 0; i < TLB_SIZE; i++) {
                            if (tlb[i].valid && tlb[i].page_number == old_page) {
                                tlb[i].valid = false;
                                break;
                            }
                        }
                    }
                }
                
                // Copy page into physical memory frame
                for (int i = 0; i < PAGE_SIZE; i++) {
                    physical_memory[frame_number * PAGE_SIZE + i] = buffer[i];
                }
                
                // Update page table
                page_table[page_number].frame_number = frame_number;
                page_table[page_number].valid = true;
                
                // Update the last used time for this frame
                frame_last_used[frame_number] = current_time;
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
    
    // Cleanup
    free(physical_memory);
    free(frame_last_used);
    fclose(addresses_file);
    fclose(backing_store);
    
    return 0;
}