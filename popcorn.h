// Error codes
#define pop_SUCCESS  0        // The operation completed successfully.
#define pop_EEXISTS  -16      // The specified file or directory already exists.
#define pop_ENOPERM  -17      // You do not have enough permission to complete this operation.
#define pop_ENOENT   -18      // The specified file or directory was not found.
#define pop_EUEFI    -19      // UEFI firmware error.
#define pop_ETOOLONG -20      // The specified path length is too long.
#define pop_ENOEXEC  -21      // The specified file is not a valid executable.
#define pop_EEOF     -22      // The end of the file has been reached.

// Backwards compatibility
#define pop_ENOENTY pop_ENOENT
#define pop_E2LPATH pop_ETOOLONG
#define pop_EUEFIPR pop_EUEFI

// Type definitions
typedef unsigned short CHAR16;

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    typedef _Bool BOOL;
#else
    typedef unsigned char BOOL;
#endif

#ifndef TRUE
#define TRUE ((BOOL)1)
#endif

#ifndef FALSE
#define FALSE ((BOOL)0)
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

// Deprecation
#if defined(_MSC_VER)
    #define pop_DEPRECATED(msg) __declspec(deprecated(msg))
#elif defined(__GNUC__) && __GNUC__ >= 4
    #define pop_DEPRECATED(msg) __attribute__((deprecated(msg)))
#else
    #define pop_DEPRECATED(msg) __attribute__((deprecated))
#endif

// Popcorn OS calling convention (rcx, rdx, r8, r9, ...stack)
#ifdef _MSC_VER
    #define pop_API __fastcall
#else
    #define pop_API __attribute__((ms_abi))
#endif

// Annotations:
// - /* memfreeable */ : Must be freed using pop_Services.memfree.
// - int /* code */    : Integer. Must be a pop_* error code.
// - int /* code? */   : Integer. If positive, a regular integer. If negative, a pop_* error code.
// - /* seterr */      : Designates a function that sets pop_Services.errcode for errors.
// - /* optional */    : Designates a argument that may be NULL.

typedef struct pop_Services pop_Services;

// The mode in which a file is opened.
typedef struct popf_FileMode {
    // Read access.
    BOOL                         read;
    
    // Write access.
    BOOL                         write;
    
    // Should we create the file if it doesn't exist?
    BOOL                         create;
    
    // Should we treat the contents as bytes or text?
    BOOL                         bytes;
} popf_FileMode;

// Represents a file.
typedef struct popf_File {
    // Undocumented and changes between implementations.
    void*                        shndl;
    
    // Read from the file.
    void* /* memfreeable */      (*read)      (struct popf_File*);
    
    // Write to the file.
    int                          (*write)     (struct popf_File*, void* data);
    
    // Close the file.
    int                          (*close)     (struct popf_File*);
    
    // Pointer to the parent service struct.
    pop_Services*                svc;
    
    // The mode in which this file was opened.
    popf_FileMode                mode;
} popf_File;

// A linked list node.
typedef struct popl_Node {
    // The data stored within this node.
    void*                        data;
    
    // The next node.
    struct popl_Node*            next;
    
    // The previous node.
    struct popl_Node*            prev;
} popl_Node;

// A simple doubly-linked list service.
typedef struct popl_ListServices {
    // Pointer to the parent service table.
    pop_Services*                svc;
    
    // Creates a new instance of popl_Node.
    popl_Node* /* memfreeable */ (*newnode)   (struct popl_ListServices*, void* data);
    
    // Frees a linked list.
    void                         (*freelist)  (struct popl_ListServices*, popl_Node* head);
    
    // Appends items to a linked list.
    void                         (*append)    (struct popl_ListServices*, popl_Node* head, void* data);
} popl_ListServices;

// Represents a pixel.
typedef struct popg_Pixel {
    // Red light (0-255)
    unsigned char                r;
    
    // Green light (0-255)
    unsigned char                g;
    
    // Blue light (0-255)
    unsigned char                b;
} popg_Pixel;

// A simple framebuffer drawing service.
typedef struct popg_GraphicsServices {
    // Pointer to the parent service table.
    pop_Services*                svc;
    
    // Framebuffer of pixels.
    // DO NOT ACCESS before calling init!
    popg_Pixel*                  frame;
    
    // Width and height
    // DO NOT ACCESS before calling init!
    unsigned int                 w, h;
    
    // Initialize the screen for drawing.
    int /* code */               (*init)      (struct popg_GraphicsServices*);
    
    // Draw the pixels from the framebuffer to the screen.
    void                         (*blit)      (struct popg_GraphicsServices*);
    
    // Undocumented and changes between implementations.
    void*                        shndl;
    
    // Put a pixel onto the framebuffer.
    pop_DEPRECATED("popg_GraphicsServices.putpixel introduces another function call when it is not needed, "
                   "slowing down applications quite heavily. Use popg_PUTPIXEL instead, which is a function-"
                   "like macro and a drop-in replacement.")
    void                         (*putpixel)  (struct popg_GraphicsServices*, unsigned int x, unsigned int y,
                                               unsigned char r, unsigned char g, unsigned char b);
    
    // Get a pixel from the framebuffer.
    pop_DEPRECATED("popg_GraphicsServices.getpixel introduces another function call when it is not needed, "
                   "slowing down applications quite heavily. Use popg_GETPIXEL instead, which is a function-"
                   "like macro and a drop-in replacement.")
    popg_Pixel                   (*getpixel)  (struct popg_GraphicsServices*, unsigned int x, unsigned int y);
    
    // Deinitializes the graphics context.
    void                         (*deinit)    (struct popg_GraphicsServices*);
} popg_GraphicsServices;

/* Replacements for the deprecated putpixel and getpixel that are faster. */
#define popg_PUTPIXEL(sgfx, X, Y, R, G, B) \
    do { \
        if ((X) >= 0 && (X) < (sgfx)->w && (Y) >= 0 && (Y) < (sgfx)->h) { \
            (sgfx)->frame[(Y) * (sgfx)->w + (X)] = (popg_Pixel){ (R), (G), (B) }; \
        } \
    } while (0)
        
#define popg_GETPIXEL(sgfx, X, Y, R, G, B) \
    (sgfx)->frame[(Y) * (sgfx)->w + (X)]

// Mouse services.
// This depends on pop_Services.sgfx for the screen width & height.
typedef struct popm_MouseServices {
    // Pointer to the parent service table.
    pop_Services*                svc;
    
    // Cursor x and y.
    unsigned int                 x, y;
    
    // Query the mouse.
    int /* code */               (*query)     (struct popm_MouseServices*);
    
    // Initialize the mouse for getting input.
    int /* code */               (*init)      (struct popm_MouseServices*);
    
    // Undocumented and changes between implementations.
    void*                        shndl;
    
    // Cursor scrollwheel.
    unsigned int                 z;
    
    // Sensitivity, a number between 0 and 1 where 0 is no movement and 1 is the
    // ((screen width + screen width) / 2)px per unit of movement. Default is 0.1.
    double                       sensitivity;
    
    // Mouse buttons.
    BOOL                         left, right;
} popm_MouseServices;

typedef struct popt_Termsize {
    unsigned int rows;
    unsigned int cols;
} popt_Termsize;

// Time and date.
typedef struct popd_Datetime {
    // Year
    unsigned short int           year;
    
    // Month
    unsigned char                month;
    
    // Day
    unsigned char                day;
    
    // Hours
    unsigned char                hours;
    
    // Minutes
    unsigned char                minutes;
    
    // Seconds
    unsigned char                seconds;
    
    // Nanoseconds
    unsigned int                 nanoseconds;
} popd_Datetime;

// The main interface via which apps interact with the kernel.
typedef struct pop_Services {
    // Prints text to the console.
    void                         (*print)     (struct pop_Services* /* optional */, const CHAR16* msg);
    
    // Allocates memory.
    void* /* memfreeable */      (*memalloc)  (struct pop_Services* /* optional */, unsigned int sz);
    
    // Frees allocated memory.
    void                         (*memfree)   (struct pop_Services* /* optional */, void* /* memfreeable */ buf);
    
    // Reads a line from the console.
    CHAR16* /* memfreeable */    (*readline)  (struct pop_Services*);
    
    // Allocates a new instance of pop_Services and returns its pointer.
    struct pop_Services*         (*newsvc)    (struct pop_Services* /* optional */);
    
    // Executes a process.
    int /* code? */              (*pwait)     (struct pop_Services*, struct pop_Services* svc2, int argc, CHAR16** argv);
    
    // Opens a file.
    popf_File* /* seterr */      (*fileopen)  (struct pop_Services*, const CHAR16* path, popf_FileMode mode);
    
    // Checks if a directory exists.
    BOOL                         (*direxists) (struct pop_Services*, const CHAR16* path);
    
    // UNIMPLEMENTED: Creates a directory.
    int /* code */               (*dirmake)   (struct pop_Services*, const CHAR16* path);
    
    // The file handle to the console.
    popf_File*                   console;
    
    // The current working directory as an absolute path.
    // Please do not set this yourself; instead use setcwd.
    CHAR16*                      curdir;
    
    // Normalizes a path into an absolute one.
    CHAR16* /* memfreeable */    (*normpath)  (struct pop_Services*, const CHAR16* path);
    
    // Changes the current working directory.
    int /* code */               (*setcwd)    (struct pop_Services*, const CHAR16* path);
    
    // Prints a line of text to the console.
    void                         (*println)   (struct pop_Services* /* optional */, const CHAR16* msg);
    
    // Error code set by /* seterr */ functions.
    int /* code */               errcode;
    
    // Prints an error message from an error code (int /* code */) to the console.
    void                         (*perrno)    (struct pop_Services*, int /* code */ errcode);
    
    // Prints an integer to the console.
    void                         (*printint)  (struct pop_Services*, int num);
    
    // List services struct.
    popl_ListServices*           slist;
    
    // List the contents of a directory.
    popl_Node* /* seterr */      (*dirlist)   (struct pop_Services*, const CHAR16* path);
    
    // Frees an instance of pop_Services.
    // Please do not call svc->freesvc(svc, svc); that is undefined behaviour.
    void                         (*freesvc)   (struct pop_Services* /* optional */, struct pop_Services* svc2);
    
    // Simple graphics services.
    popg_GraphicsServices*       sgfx;
    
    // UNIMPLEMENTED: Get the width and height of the terminal.
    popt_Termsize                (*termsize)  (struct pop_Services*);
    
    // Set the terminal cursor position.
    void                         (*curmove)   (struct pop_Services*, unsigned int x, unsigned int y);
    
    // Simple mouse services.
    popm_MouseServices*          msvc;
    
    // Reboot the system. quick specifies if it's a quick reset or a full reboot.
    void                         (*reset)     (struct pop_Services* /* optional */, BOOL quick);
    
    // Get the date and time.
    popd_Datetime                (*datetime)  (struct pop_Services* /* optional */);
    
    // Get the system uptime in nanoseconds.
    unsigned long long           (*uptimens)  (struct pop_Services* /* optional */);
    
    // Prints an unsigned long long int to the console.
    void                         (*printull)  (struct pop_Services*, unsigned long long num);
} pop_Services;