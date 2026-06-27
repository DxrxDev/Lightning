#define __BASE_IMPLEMENTATION
#define MMM_IMPLEMENTATION
#include "wingfx.h"
#include "logic.h"
#include "geometry.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>

#define XK_MISCELLANY
#include <X11/keysym.h>

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xcb.h>

#if defined(__linux__)

#define SURFACE_PLATFORM_VK_EXTENSION VK_KHR_XCB_SURFACE_EXTENSION_NAME

#elif defined( _WIN32 )
    #error "WINDOWS NOT SUPPORTED"
#else
    #error "NO (valid) OPERATING SYSTEM DETECTED"
#endif

static struct WingFX_instance{
    bool running;
    const char *title;
    bool keys[UINT8_MAX];
    bool mouse[NUMBER_MOUSE_BUTTONS];

    uint32_t width, height;

    xcb_window_t  window;
    xcb_atom_t deletewin, protocols;
    xcb_connection_t *conn;
    xcb_screen_t *screen;
    xcb_key_symbols_t* symbols;

} wfx;

Return_t InitGraphics( );
void InitWindow( uint32_t width, uint32_t height, const char *title, enum WindowCreationFlags flag ){
    wfx.title = title;

    wfx.conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(wfx.conn)){
        ExitOnError("Couldn't establish XCB connection");
    }
    wfx.screen = xcb_setup_roots_iterator( xcb_get_setup(wfx.conn) ).data;

    wfx.window = xcb_generate_id(wfx.conn);

    uint32_t windowMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t windowMaskValues[] = {
        wfx.screen->black_pixel,
        (
            XCB_EVENT_MASK_EXPOSURE |
            XCB_EVENT_MASK_KEY_PRESS |
            XCB_EVENT_MASK_KEY_RELEASE |
            XCB_EVENT_MASK_POINTER_MOTION |
            XCB_EVENT_MASK_BUTTON_PRESS |
            XCB_EVENT_MASK_BUTTON_RELEASE |
            XCB_EVENT_MASK_STRUCTURE_NOTIFY
        )
    };

    xcb_create_window(
        wfx.conn,
        XCB_COPY_FROM_PARENT,
        wfx.window,
        wfx.screen->root,
        400, 200,
        width, height,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        wfx.screen->root_visual,
        windowMask, windowMaskValues
    );

    if (!(flag & WCF_Resizable)){
        xcb_size_hints_t hints;
        xcb_icccm_size_hints_set_min_size(&hints, width, height);
        xcb_icccm_size_hints_set_max_size(&hints, width, height);
        xcb_icccm_size_hints_set_size(&hints, 1, width, height);

        xcb_icccm_set_wm_size_hints(wfx.conn, wfx.window, XCB_ATOM_WM_NORMAL_HINTS, &hints);
    }

    xcb_intern_atom_reply_t *deletereply = xcb_intern_atom_reply(
        wfx.conn,
        xcb_intern_atom(wfx.conn, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW"),
        NULL
    );
    xcb_intern_atom_reply_t *protocolsreply = xcb_intern_atom_reply(
        wfx.conn,
        xcb_intern_atom(wfx.conn, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS"),
        NULL
    );
    wfx.deletewin = deletereply->atom;
    wfx.protocols = protocolsreply->atom;

    xcb_change_property(
        wfx.conn, XCB_PROP_MODE_REPLACE, wfx.window,
        wfx.protocols, 4, 32, 1, &wfx.deletewin
    );
    free(deletereply);
    free(protocolsreply);

    xcb_map_window( wfx.conn, wfx.window );
    xcb_flush( wfx.conn );

    xcb_change_property(
        wfx.conn,
        XCB_PROP_MODE_REPLACE,
        wfx.window,
        XCB_ATOM_WM_NAME,
        XCB_ATOM_STRING,
        8,
        strlen(title),
        title
    );
    xcb_flush( wfx.conn );

    wfx.symbols = xcb_key_symbols_alloc(wfx.conn);

    wfx.running = true;

    ExitOnError(InitGraphics( ));
}

void *GetWindowSystem( void ){
    return wfx.conn;
}

void TerminateGraphics( );
void TerminateApp( ){
    xcb_key_symbols_free(wfx.symbols);
    xcb_destroy_window( wfx.conn, wfx.window );
    xcb_flush( wfx.conn );

    exit(0);
}

nodisc bool AppRunning( ){
    return wfx.running;
}

nodisc WindowEvent *GetWindowEvents( void ){
    xcb_generic_event_t *event;
    WindowEvent *ret = malloc(sizeof(WindowEvent));
    ret->type = WET_Start;
    WindowEvent *on = ret;
    while ((event = xcb_poll_for_event(wfx.conn))){
        switch (event->response_type & ~0x80){
            case XCB_DESTROY_NOTIFY:{
                xcb_destroy_notify_event_t *e =
                    (xcb_destroy_notify_event_t *)event;

                if (e->window != wfx.window)
                    break;
                wfx.running = false; 
            } break;
            case XCB_CLIENT_MESSAGE:{
                xcb_client_message_event_t *e =
                    (xcb_client_message_event_t *)event;
                
                if (e->window != wfx.window)
                    break;

                if (e->data.data32[0] == wfx.deletewin)
                    wfx.running = false;
            }; break;
            case XCB_KEY_PRESS:{
                xcb_key_press_event_t e = *(xcb_key_press_event_t *)event;
                    
                xcb_keysym_t ks;
                if (e.detail == 0x32)
                    break;
                    
                ks = xcb_key_symbols_get_keysym(wfx.symbols, e.detail, 0);
                    
                ret->next = malloc(sizeof(WindowEvent));
                on = ret->next;

                *on = (WindowEvent){
                    WET_Key,
                    {.key = (struct WE_Key){
                        ks,
                        true, false, false
                    }},
                    0
                };

                if (ks > 0 && ks < UCHAR_MAX)
                    wfx.keys[ks] = true;
                
            } break;
            case XCB_KEY_RELEASE:{
                xcb_key_release_event_t e = *(xcb_key_release_event_t *)event;
                    
                xcb_keysym_t ks;
                if (e.detail == 0x32)
                    break;

                ks = xcb_key_symbols_get_keysym(wfx.symbols, e.detail, 0);
                    
                ret->next = malloc(sizeof(WindowEvent));
                on = ret->next;

                *on = (WindowEvent){
                    WET_Key,
                    {.key = (struct WE_Key){
                        ks,
                        false, false, false
                    }},
                    0
                };

                if (ks > 0 && ks < UCHAR_MAX)
                    wfx.keys[ks] = false;
            } break;
            case XCB_BUTTON_PRESS:{
                xcb_button_press_event_t e = *(xcb_button_press_event_t *)event;
                xcb_button_t button = e.detail;
                    
                ret->next = malloc(sizeof(WindowEvent));
                on = ret->next;


                uint32_t buttontype;
                switch (button){
                    case 1:{buttontype = MB_Left;} break;
                    case 2:{buttontype = MB_Middle;} break;
                    case 3:{buttontype = MB_Right;} break;
                }

                    
                *on = (WindowEvent){
                    WET_MouseClk,
                    {.mc = (struct WE_MouseClk){
                        buttontype, true
                    }},
                    0
                };
            } break;
            case XCB_BUTTON_RELEASE:{
                xcb_button_release_event_t e = *(xcb_button_release_event_t *)event;
                xcb_button_t button = e.detail;
                    
                ret->next = malloc(sizeof(WindowEvent));
                on = ret->next;

                uint32_t buttontype;
                switch (button){
                    case 1:{buttontype = MB_Left;} break;
                    case 2:{buttontype = MB_Middle;} break;
                    case 3:{buttontype = MB_Right;} break;
                }

                *on = (WindowEvent){
                    WET_MouseClk,
                    {.mc = (struct WE_MouseClk){
                        buttontype, false
                    }},
                    0
                };
            } break;
            case XCB_MOTION_NOTIFY:{
                xcb_motion_notify_event_t e = *(xcb_motion_notify_event_t*)event;
                if (e.event != wfx.window) break;

                ret->next = malloc(sizeof(WindowEvent));
                on = ret->next;

                *on = (WindowEvent){
                    WET_MouseMove,
                    {.mm = (struct WE_MouseMove){
                        (float)e.event_x,
                        (float)e.event_y,
                        (float)e.root_x,
                        (float)e.root_y
                    }},
                    0
                };
            } break;
            case XCB_CONFIGURE_NOTIFY:{
                xcb_configure_notify_event_t e = *(xcb_configure_notify_event_t*)event;
                if (e.event != wfx.window) break;

                ret->next = malloc(sizeof(WindowEvent));
                on = ret->next;

                *on = (WindowEvent){
                    WET_Geometry,
                    {.mm = (struct WE_MouseMove){
                        (float)e.x,
                        (float)e.y,
                        (float)e.width,
                        (float)e.height
                    }},
                    0
                };
                wfx.width = e.width;
                wfx.height = e.height;
            } break;
            default: {
                printf("Unhandled Window Event: %d\n", (event->response_type & ~0x80));
            } break;
            }
        }
        on->next = malloc(sizeof(WindowEvent));
        on->next->type = WET_End;

        xcb_flush( wfx.conn );

        return ret;
}
void ClearWindowEvents( WindowEvent *events ){
    if (events->type != WET_End){
        ClearWindowEvents( events->next );
    }
    free(events);
}

bool window_key_down(char c){
    if (0 < c && c < UCHAR_MAX)
        return wfx.keys[(uint32_t)c];
    return false;
}

Vector2 window_centre( void ){
    return (Vector2){wfx.width, wfx.height};
}

typedef struct GraphicsBuffer{
    VkBuffer buffer;
    VkDeviceMemory memory;

    uint32_t sizeofdata;
    uint32_t sizeondevice;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags properties;

} GraphicsBuffer;

/*
typedef struct StackedBuffer {
    BA buffers; // VkBuffer
    VkDeviceMemory memory;

    uint32_t sizeofdata, sizeondevice;

    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags properties;
} StackedBuffer;
*/

typedef struct UniformBuffer {
    GraphicsBuffer buf;
    uint32_t bin, loc;
} UniformBuffer;

struct resbuf {
    bool hc;
    uint32_t offset;
    uint32_t size;
};
typedef struct DrawerDef {
    VkPipeline pipeline; 
    VkPipelineLayout pipelinelayout;

    VkDescriptorPool      discpool;
    VkDescriptorSet       discset;
    VkDescriptorSetLayout disclayout;
  
    GraphicsBuffer buffers, hcbuffers;
    size_t vsize, voffset, isize, ioffset;
    BA discoffsets;
} DrawerDef;

typedef struct DrawableDef {
    uint32_t ref;
    Drawer drawer;
} DrawableDef;

static struct Graphics_instance{
    VkInstance   instance;
    VkSurfaceKHR surface;
    VkExtent2D   extent;
    
    VkPhysicalDevice physicaldevice;
    VkDevice         device;
    uint32_t         graphics_i, present_i;
    VkQueue          graphics,   present;

    VkSharingMode      sharingmode;
    VkSurfaceFormatKHR surfaceformat;
    VkSwapchainKHR     swapchain;
    uint32_t           imagecount;
    VkImageView       *images;
    VkFramebuffer     *frames;

    VkImage depthstencilimage;
    VkImageView depthstencilimageview;
    VkDeviceMemory depthstencilmemory;

    DrawerDef *drawers;
    uint32_t drawersused;
    uint32_t drawercount;
    
    VkDescriptorSetLayout  uniformlayout, texturelayout;
    VkDescriptorSet        uniform, texture;
    VkDescriptorPool       descriptorpool;
    VkRenderPass           renderpass;

    VkCommandPool commandpool;
    VkCommandBuffer drawcommand;

    VkSemaphore semimagegrabbed;
    VkSemaphore semrenderdone;
    VkFence feninflight;

    CameraInfo cam;
} gfx;

typedef struct MeshMemory{
    int64_t start, end;
    uint32_t *inds, indcount;
    struct MeshMemory *prev, *next;
    bool visable; DrawableDef dr;
} MeshMemory;
struct {
    GraphicsBuffer vtx, ind;
    MeshMemory vtxmem; bool memupdated;
    ECS_t refs, transforms;
} bufman;

struct {
    VkImage image;
    VkImageView imageview;
    VkSampler sampler;
    VkDeviceMemory memory;
} samplers;

Return_t CreateInstance();
Return_t CreateDevice();
Return_t CreateSwapchain();
Return_t CreateLayouts();
Return_t CreateRenderPass();
Return_t CreateCommands();
Return_t CreateSyncVars();
Return_t CreateBuffers();
Return_t CreateImageSamplers();

Return_t CreateUiLayouts();
Return_t CreateUiPipeline();
Return_t CreateUiBuffers();

void UpdateVertexBuffer( Drawer drawer, MeshResource_t t, uint32_t offset );
void UpdateIndexBuffer( Drawer drawer, MeshResource_t t, uint32_t offset );

bool CheckSuitabilityOfDevice( VkPhysicalDevice dev, VkSurfaceKHR sur, uint32_t *graphics, uint32_t *present ){
    uint32_t g, p;
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures   deviceFeatures;
    vkGetPhysicalDeviceProperties( dev, &deviceProperties );
    vkGetPhysicalDeviceFeatures( dev, &deviceFeatures );

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, 0);
    VkQueueFamilyProperties queueFamilies[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies);

    int32_t graphicsFound = false;
    VkBool32 presentFound = false;
    for (uint32_t i = 0; i < queueFamilyCount; ++i){
        printf("queue %d\n", i);

        if (!graphicsFound){
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
                g = i;
                graphicsFound = true;
                    printf( "found gfx at %d\n", i);
            }
        }

        if (!presentFound){
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, sur, &presentFound);
            if (presentFound){
                p = i;
                    printf( "found prs at %d\n", i);
            }
        }

        if (graphicsFound && presentFound){
            break;
        }
    }
    if (!graphicsFound){
        return false;
    }
    if (presentFound == VK_FALSE){
        return false;
    }

    *graphics=g;
    *present =p;

    return true;
}
uint32_t FindMemoryType( uint32_t filter, VkMemoryPropertyFlags properties ){
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(gfx.physicaldevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i){
        if ((filter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    printf("Couldn't find memory type for buffer :/\n");
    return 0;
}

void CreateBuffer(GraphicsBuffer *buffer, uint32_t datasize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties){
    buffer->sizeofdata = datasize;  
    
    VkBufferCreateInfo bci = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = datasize,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    vkCreateBuffer( gfx.device, &bci, 0, &buffer->buffer );

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements( gfx.device, buffer->buffer, &memRequirements);
    buffer->sizeondevice = memRequirements.size;

    VkMemoryAllocateInfo memoryAI = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = 0,
        .allocationSize = buffer->sizeondevice,
        .memoryTypeIndex = FindMemoryType(
            memRequirements.memoryTypeBits,
            properties
        )
    };
    if (vkAllocateMemory(gfx.device, &memoryAI, 0, &buffer->memory) != VK_SUCCESS) {
        buffer = VK_NULL_HANDLE;
        return;
    }
    vkBindBufferMemory(gfx.device, buffer->buffer, buffer->memory, 0);
}
void DestroyBuffer( GraphicsBuffer *buffer ){
    if (buffer == 0)
        return;
    vkDestroyBuffer( gfx.device, buffer->buffer, 0 );
    vkFreeMemory( gfx.device, buffer->memory, 0 );
}

/* OVERCOMPLICATED?
void StackedBufferCreate(StackedBuffer *bufs, uint32_t *sizes, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties){
    uint32_t totalsize = 0;
    uint32_t bufcount = 0;
    while (sizes[bufcount]){
        totalsize += sizes[bufcount];
        ++bufcount;
    }

    bufs->buffers = BACreate(sizeof(VkBuffer), bufcount);
    VkBufferCreateInfo bci = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .size = 0, // To be set in following loop
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    for (uint32_t i = 0; i < bufcount; ++i){
        bci.size = sizes[i];
        VkCreateBuffer( gfx.device, &bci, 0, BAGetPointer( bufs->buffers, 0 ) );
    }

}
*/

Return_t InitGraphics( ){
    Return_t r;
    r = CreateInstance();
    if (r != 0){
        return r;
    }
    r = CreateDevice();
    if (r != 0){
        return r;
    }
    r = CreateSwapchain();
    if (r != 0){
        return r;
    }
    r = CreateLayouts();
    if (r != 0){
        return r;
    }
    r = CreateRenderPass();
    if (r != 0){
        return r;
    }
    //r = CreatePipeline();
    //if (r != 0){
    //    return r;
    //}
    r = CreateCommands();
    if (r != 0){
        return r;
    }
    r = CreateSyncVars();
    if (r != 0){
        return r;
    }
    r = CreateBuffers();
    if (r != 0){
        return r;
    }
    r = CreateImageSamplers();
    if (r != 0){
        return r;
    }

    // VkPhysicalDeviceProperties props;
    // VkPhysicalDeviceLimits limits;
    // vkGetPhysicalDeviceProperties(gfx.physicaldevice, &props);
    // limits = props.limits;
    // printf("Max memory allocations %lu\n", limits.sparseAddressSpaceSize);
    gfx.drawers = malloc(sizeof(DrawerDef) * 2);
    gfx.drawersused = 0;
    gfx.drawercount = 2; // TODO: fix this lol

    return 0;
}
Return_t CreateInstance( ){
    gfx.instance = 0;
    const char *layers[1] = {
        "VK_LAYER_KHRONOS_validation"
    };
    const char *extensions[2] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        SURFACE_PLATFORM_VK_EXTENSION
    };
    VkApplicationInfo appinfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = wfx.title,
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName = "zubway_engine",
        .engineVersion = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion = VK_API_VERSION_1_0
    };
    VkInstanceCreateInfo createinfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .pApplicationInfo = &appinfo,
        .enabledLayerCount = 1,
        .ppEnabledLayerNames = layers,
        .enabledExtensionCount = 2,
        .ppEnabledExtensionNames = extensions
    };
    if (vkCreateInstance(&createinfo, 0, &gfx.instance) != VK_SUCCESS){
        return "Couldnt create vulkan instance...";
    }

    VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        .pNext = 0,
        .flags = 0,
        .connection = wfx.conn,
        .window = wfx.window
    };

    if (vkCreateXcbSurfaceKHR( gfx.instance, &surfaceCreateInfo, NULL, &gfx.surface ) != VK_SUCCESS){
        return "Error Couldnt Create Surface!!";
    }

    xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(
        wfx.conn,
        xcb_get_geometry (wfx.conn, wfx.window), 
        NULL
    );
    gfx.extent.width = geom->width;
    gfx.extent.height = geom->height;
    free( geom );

    return 0;
}
Return_t CreateDevice( ){
    uint32_t numphysicaldevices = 0;
    vkEnumeratePhysicalDevices(gfx.instance, &numphysicaldevices, 0);
    VkPhysicalDevice physicaldevices[numphysicaldevices];
    vkEnumeratePhysicalDevices(gfx.instance, &numphysicaldevices, physicaldevices);

    uint32_t validat = UINT32_MAX;
    for (uint32_t i = 0; i < numphysicaldevices; ++i){
        if (CheckSuitabilityOfDevice( physicaldevices[i], gfx.surface, &gfx.graphics_i, &gfx.present_i )){
            validat = i;
            break;
        }
    }
    if (validat == UINT32_MAX){
        return "Couldn't find a suitable device!";
    }

    gfx.sharingmode = VK_SHARING_MODE_EXCLUSIVE;
    gfx.physicaldevice = physicaldevices[validat];

    float queuepriorities[1] = {1.0f};
    VkDeviceQueueCreateInfo devicequeuecreateinfos[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = gfx.graphics_i,
            .queueCount = 1,
            .pQueuePriorities = queuepriorities
        },
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = gfx.present_i,
            .queueCount = 1,
            .pQueuePriorities = queuepriorities
        }
    };

    if (gfx.graphics_i != gfx.present_i){
        gfx.sharingmode = VK_SHARING_MODE_CONCURRENT;
    }

    VkDeviceCreateInfo devicecreateinfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = (gfx.graphics_i==gfx.present_i) ? 1 : 2,
        .pQueueCreateInfos = devicequeuecreateinfos,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = (const char*[]){ VK_KHR_SWAPCHAIN_EXTENSION_NAME }
    };

    if (vkCreateDevice(
        gfx.physicaldevice,
        &devicecreateinfo,
        0,
        &gfx.device
    ) != VK_SUCCESS){
        return "Couldn't create device!";
    }

    vkGetDeviceQueue(gfx.device, gfx.graphics_i, 0, &gfx.graphics);
    vkGetDeviceQueue(gfx.device, gfx.present_i, 0, &gfx.present);

    return 0;
}
Return_t CreateSwapchain( ){
    uint32_t minimagecount;
    VkPresentModeKHR presentmode;

    VkSurfaceCapabilitiesKHR surfacecapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gfx.physicaldevice, gfx.surface, &surfacecapabilities);

    if (surfacecapabilities.minImageCount == 1)
        minimagecount = surfacecapabilities.minImageCount + 1;
    else 
        minimagecount = surfacecapabilities.minImageCount;

    if (minimagecount > surfacecapabilities.maxImageCount && surfacecapabilities.maxImageCount)
        minimagecount = surfacecapabilities.maxImageCount;

    printf("Swachain has at least %d images\n", minimagecount );

    uint32_t surfaceformatcount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gfx.physicaldevice, gfx.surface, &surfaceformatcount, 0);
    VkSurfaceFormatKHR availableformats[surfaceformatcount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(gfx.physicaldevice, gfx.surface, &surfaceformatcount, availableformats);
    
    uint32_t usingformat = 0;
    for (uint32_t i = 0; i < surfaceformatcount; ++i){
        VkSurfaceFormatKHR sformat = availableformats[i];
        if (sformat.format == VK_FORMAT_B8G8R8A8_SRGB && sformat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            usingformat = i;
    }
    gfx.surfaceformat = availableformats[usingformat];
    
    if (surfacecapabilities.currentExtent.width != UINT32_MAX){
        gfx.extent = surfacecapabilities.currentExtent;
    }

    uint32_t presmodecount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(gfx.physicaldevice, gfx.surface, &presmodecount, 0);
    VkPresentModeKHR presentmodes[presmodecount];
    vkGetPhysicalDeviceSurfacePresentModesKHR(gfx.physicaldevice, gfx.surface, &presmodecount, presentmodes);
    presentmode = VK_PRESENT_MODE_FIFO_KHR;
    //for (VkPresentModeKHR presentPresent : presentModes){
    //    if (presentPresent == VK_PRESENT_MODE_MAILBOX_KHR)
    //        presentmode = VK_PRESENT_MODE_MAILBOX_KHR;
    //}

    uint32_t queuefamilies[2] = {
        gfx.graphics_i, gfx.present_i
    };
    VkSwapchainCreateInfoKHR sci = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = gfx.surface,
        .minImageCount = minimagecount,
        .imageFormat = gfx.surfaceformat.format,
        .imageColorSpace = gfx.surfaceformat.colorSpace,
        .imageExtent = gfx.extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = gfx.sharingmode,
        .queueFamilyIndexCount = (gfx.graphics_i==gfx.present_i) ? 1 : 2,
        .pQueueFamilyIndices = queuefamilies,
        .preTransform = surfacecapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentmode,
        .clipped = VK_TRUE,
        .oldSwapchain = 0
    };

    if (vkCreateSwapchainKHR(gfx.device, &sci, 0, &gfx.swapchain) != VK_SUCCESS){
        return "Failed creating the swapchain";
    }

    gfx.imagecount = 0;
    vkGetSwapchainImagesKHR(gfx.device, gfx.swapchain, &gfx.imagecount, 0);
    VkImage scimages[gfx.imagecount];
    vkGetSwapchainImagesKHR(gfx.device, gfx.swapchain, &gfx.imagecount, scimages);

    gfx.images = malloc(sizeof(VkImageView) * gfx.imagecount);

    for (uint32_t i = 0; i < gfx.imagecount; ++i){
        VkImageViewCreateInfo ivci = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = scimages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = gfx.surfaceformat.format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        vkCreateImageView( gfx.device, &ivci, 0, gfx.images + i );
    }

    return 0;
}
Return_t CreateLayouts( ){
    VkDescriptorSetLayoutBinding dslbUB = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr
    };
    VkDescriptorSetLayoutCreateInfo ulciUB = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 1,
        .pBindings = &dslbUB
    };

    VkDescriptorSetLayoutBinding dslbS = {
        .binding = 0, // CHANGEDDD 
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr
    };
    VkDescriptorSetLayoutCreateInfo ulciS = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 1,
        .pBindings = &dslbS
    };

    if (vkCreateDescriptorSetLayout(gfx.device, &ulciUB, nullptr, &gfx.uniformlayout) != VK_SUCCESS){
        return "Couldn't create descriptor set layout! (1)";
    }
    if (vkCreateDescriptorSetLayout(gfx.device, &ulciS, nullptr, &gfx.texturelayout) != VK_SUCCESS){
        return "Couldn't create descriptor set layout! (2)";
    }

    VkDescriptorPoolSize poolsizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1
        }
    };
    VkDescriptorPoolCreateInfo pci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = 5,
        .poolSizeCount = 2,
        .pPoolSizes = poolsizes,
    };
    if (vkCreateDescriptorPool(gfx.device, &pci, 0, &gfx.descriptorpool) != VK_SUCCESS){
        return "Couldn't create descriptor pool!";
    }

    return 0;
}
Return_t CreateRenderPass( ){
    VkAttachmentDescription colordesc = {
		.flags = 0,
    	.format = gfx.surfaceformat.format,
    	.samples = VK_SAMPLE_COUNT_1_BIT,
    	.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    	.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    	.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    	.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    	.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    	.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};
    VkAttachmentReference colorref = {
    	.attachment = 0, // Index of the color attachment
    	.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

    VkAttachmentDescription depthstencildesc = {
        .flags = 0,
        .format = VK_FORMAT_D32_SFLOAT_S8_UINT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    VkAttachmentReference depthstencilref = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
    	.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    	.colorAttachmentCount = 1,
    	.pColorAttachments = &colorref,
        .pDepthStencilAttachment = &depthstencilref
	};

    VkSubpassDependency dependency = {
    	.srcSubpass = VK_SUBPASS_EXTERNAL,
    	.dstSubpass = 0,
    	.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    	.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    	.srcAccessMask = 0,
    	.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
	};

    VkAttachmentDescription attachmentdescriptions[] = {colordesc, depthstencildesc};
    VkRenderPassCreateInfo rpci = {
    	.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
    	.attachmentCount = 2,
    	.pAttachments = attachmentdescriptions,
    	.subpassCount = 1,
    	.pSubpasses = &subpass,
    	.dependencyCount = 1,
    	.pDependencies = &dependency
	};

    vkCreateRenderPass(gfx.device, &rpci, nullptr, &gfx.renderpass);

    VkResult res;

    VkImageCreateInfo ici = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_D32_SFLOAT_S8_UINT,
        .extent = (VkExtent3D){
            .width = gfx.extent.width,
            .height = gfx.extent.height,
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &gfx.graphics_i,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    res = vkCreateImage( gfx.device, &ici, 0, &gfx.depthstencilimage );
    if (res != VK_SUCCESS)
        ExitOnError("Couldnt create ds image\n");

    VkMemoryRequirements memoryreq;
    vkGetImageMemoryRequirements( gfx.device, gfx.depthstencilimage, &memoryreq );
    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryreq.size,
        .memoryTypeIndex = FindMemoryType(
            memoryreq.memoryTypeBits, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        )
    };
    
    if (vkAllocateMemory( gfx.device, &allocInfo, 0, &gfx.depthstencilmemory) != VK_SUCCESS) {
        printf("Couldnt allocate ds image\n");
    }
    res = vkBindImageMemory(gfx.device, gfx.depthstencilimage, gfx.depthstencilmemory, 0);
    if (res != VK_SUCCESS)
        printf("Couldnt bind ds image\n");


    VkImageViewCreateInfo ivci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = gfx.depthstencilimage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_D32_SFLOAT_S8_UINT,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    res = vkCreateImageView( gfx.device, &ivci, 0, &gfx.depthstencilimageview );
    if (res != VK_SUCCESS)
        printf( "couldnt create image view\n" );

    gfx.frames = malloc(sizeof(VkFramebuffer) * gfx.imagecount);
    for (uint32_t i = 0; i < gfx.imagecount; ++i){
        VkImageView views[2] = {
            gfx.images[i],
            gfx.depthstencilimageview
        };
        VkFramebufferCreateInfo fci = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .renderPass = gfx.renderpass,
            .attachmentCount = 2,
            .pAttachments = views,
            .width = gfx.extent.width,
            .height = gfx.extent.height,
            .layers = 1
        }; 

        vkCreateFramebuffer(gfx.device, &fci, 0, gfx.frames + i);
    }

    return 0;
}

Return_t CreateCommands( ){

    VkCommandPoolCreateInfo cpci = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = 0,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = gfx.graphics_i
    };
    vkCreateCommandPool( gfx.device, &cpci, 0, &gfx.commandpool );

    VkCommandBufferAllocateInfo cbai = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = 0,
        .commandPool = gfx.commandpool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    vkAllocateCommandBuffers( gfx.device, &cbai, &gfx.drawcommand );

    return 0;
}

Return_t CreateSyncVars( ){
    VkSemaphoreCreateInfo sci = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };
    VkFenceCreateInfo fci = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    vkCreateSemaphore( gfx.device, &sci, 0, &gfx.semimagegrabbed );
    vkCreateSemaphore( gfx.device, &sci, 0, &gfx.semrenderdone );
    vkCreateFence( gfx.device, &fci, 0, &gfx.feninflight );


    vkDeviceWaitIdle( gfx.device );
    return 0;
}

Return_t CreateBuffers( ){
    bufman.memupdated = true;
    ComponentDefine gfxdefs[] = {
        {"t", sizeof(Matrix), 0}, // Transforms
        {0, 0, 0}
    };
    bufman.transforms = ECS_Create(1024, gfxdefs);

    ComponentDefine refdefs[] = {
        {"t", sizeof(uint32_t), 0},
        {"m", sizeof(uint32_t), 0},
        {0, 0, 0}
    };
    bufman.refs = ECS_Create(1024, refdefs);
    
    CreateBuffer(
        &bufman.vtx,
        10000 * sizeof(Vertex),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    bufman.vtxmem = (MeshMemory){
        -1, -1,
        0, 0,
        0, 0,
        true, UINT32_MAX
    };
    CreateBuffer(
        &bufman.ind,
        10000 * sizeof(uint32_t),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    
    return 0;
}

Return_t CreateImageSamplers( ){
    DirectImage readimage = directimage_create_bmp("texture_map.bmp");
    uint32_t
        width = readimage.width,
        height = readimage.height;
        
    uint32_t texsize = width * height * 4;

    VkExtent3D sampleriamgeextent = {
        width, height, 1
    };
    VkImageCreateInfo ici = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .extent = sampleriamgeextent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // TODO: not assume
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &gfx.graphics_i,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    vkCreateImage( gfx.device, &ici, 0, &samplers.image);
    VkMemoryRequirements memreq;
    vkGetImageMemoryRequirements( gfx.device, samplers.image, &memreq);

    VkMemoryAllocateInfo mai = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = 0,
        .allocationSize = memreq.size,
        .memoryTypeIndex = FindMemoryType(
            memreq.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        )
    };
    vkAllocateMemory( gfx.device, &mai, 0, &samplers.memory );
    vkBindImageMemory( gfx.device, samplers.image, samplers.memory, 0 );

    VkImageViewCreateInfo ivci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = samplers.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    vkCreateImageView( gfx.device, &ivci, 0, &samplers.imageview );

    GraphicsBuffer interbuffer;
    CreateBuffer(
        &interbuffer,
        texsize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    
    void *texdata;
    vkMapMemory( gfx.device, interbuffer.memory, 0, texsize, 0, &texdata );
        memcpy(texdata, readimage.data, texsize);
    printf("got here, memory set! %d\n", __LINE__);
    vkUnmapMemory(gfx.device, interbuffer.memory);

    VkCommandBuffer intercommand;
    VkCommandBufferAllocateInfo cbai = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = 0,
        .commandPool = gfx.commandpool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    vkAllocateCommandBuffers( gfx.device, &cbai, &intercommand );
    VkCommandBufferBeginInfo cbbi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = 0,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = 0
    };
    vkBeginCommandBuffer(intercommand, &cbbi);

    VkImageMemoryBarrier imbb = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = 0,
        .srcAccessMask = 0,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, // Ignoring anything on image
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, // Not swapping queue
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, // ownership of img
        .image = samplers.image,
        .subresourceRange = (VkImageSubresourceRange){
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    VkImageMemoryBarrier imba = imbb;
    imba.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imba.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vkCmdPipelineBarrier(
        intercommand,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, 0,
        0, 0,
        1, &imbb
    );
    VkBufferImageCopy bic = {
        .bufferOffset = 0,
        .bufferRowLength = 0,   // ASSUME THE EXTENT
        .bufferImageHeight = 0, // IS THE TEXTURE SIZE
        .imageSubresource = (VkImageSubresourceLayers){
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .imageOffset = 0,
        .imageExtent = (VkExtent3D){
            .width = width,
            .height = height,
            .depth = 1
        }
    };
    vkCmdCopyBufferToImage(
        intercommand,
        interbuffer.buffer,
        samplers.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &bic
    );

    vkCmdPipelineBarrier(
        intercommand,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, 0,
        0, 0,
        1, &imba
    );

    vkEndCommandBuffer(intercommand);
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = 0,
        .pWaitDstStageMask = 0,
        .commandBufferCount = 1,
        .pCommandBuffers = &intercommand,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = 0
    };
    vkQueueSubmit(gfx.graphics, 1, &submitInfo, VK_NULL_HANDLE);

    struct VkSamplerCreateInfo sci = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 0.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE
    };
    vkCreateSampler( gfx.device, &sci, nullptr, &samplers.sampler );
    
    
    VkDescriptorSetAllocateInfo sdai = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = 0,
        .descriptorPool = gfx.descriptorpool,
        .descriptorSetCount = 1,
        .pSetLayouts = &gfx.texturelayout
    };
    if (vkAllocateDescriptorSets(gfx.device, &sdai, &gfx.texture) != VK_SUCCESS) {
        ExitOnError("Couldn't allocate descriptor set for texture sampler!");
    }
    
    VkDescriptorImageInfo samplerinfo = {
        .sampler = samplers.sampler,
        .imageView = samplers.imageview,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    VkWriteDescriptorSet descriptorwrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = 0,
        .dstSet = gfx.texture,
        .dstBinding = 0, // CHANGEDDD
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &samplerinfo,
        .pBufferInfo = 0,
        .pTexelBufferView = 0
    };
    // Update the descriptor set to bind the uniform buffer
    vkUpdateDescriptorSets(gfx.device, 1, &descriptorwrite, 0, 0);
    
    return 0;
}

Drawer DrawerCreate( DrawerCreateInfo dci ){
    if(gfx.drawersused < gfx.drawercount){
        gfx.drawersused++;
    }
    else {
        return NULL;
    }
    Drawer currdraw = gfx.drawers + (gfx.drawersused-1);
    uint32_t totalbuffersize = 0;
    uint32_t totalhcbuffersize = 0;
    
    BADefine discoffsetsdefine = { sizeof(struct resbuf), dci.resourceinfocount };
    currdraw->discoffsets = BACreate( discoffsetsdefine );

    /* SHADERS */

    char *shadersources[2];
    int   shadersourcesizes[2];
    int   shadercount = 2;
    {
        const char *filenames[2] = {dci.vshader, dci.fshader};

        // TODO implement file safe guarding
        for (int i = 0; i < shadercount; ++i){
            FILE *fileread = fopen(filenames[i], "rb");
            fseek(fileread, 0, SEEK_END);
            shadersourcesizes[i] = ftell(fileread);
            fseek(fileread, 0, SEEK_SET);

            shadersources[i] = malloc(sizeof(char) * shadersourcesizes[i]);
            fread(shadersources[i], 1, shadersourcesizes[i], fileread);
            fclose(fileread);
        }
    }

    VkShaderModule shadermodules[2];
    {
        VkShaderModuleCreateInfo smci[2] = {
            {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .codeSize = shadersourcesizes[0],
                .pCode = (const uint32_t*)shadersources[0]
            },
            {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .codeSize = shadersourcesizes[1],
                .pCode = (const uint32_t*)shadersources[1]
            }
        };

        vkCreateShaderModule(gfx.device, smci + 0, nullptr, shadermodules + 0);
        vkCreateShaderModule(gfx.device, smci + 1, nullptr, shadermodules + 1);
    }

    VkPipelineShaderStageCreateInfo sci[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = shadermodules[0],
            .pName = "main",
            .pSpecializationInfo = nullptr
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = shadermodules[1],
            .pName = "main",
            .pSpecializationInfo = nullptr
        }
    };
    
    /* INPUT ASSEMBLY */

    uint32_t bindingstrides[16]; // assuming noone is going to use more than 16 bindings
    uint32_t bindingcount = dci.vertexinfos[0].binding;
    for (uint32_t i = 0; i < dci.vertexinfocount; ++i){
        if (dci.vertexinfos[i].binding > bindingcount)
            bindingcount = dci.vertexinfos[i].binding;
    }
    // assuming attributes are provided with binding in ascending order.

    VkVertexInputBindingDescription bindingdescs[bindingcount + 1];
    for (uint32_t b = 0; b < bindingcount + 1; ++b){
        bindingdescs[b] = (VkVertexInputBindingDescription){
            .binding = b,
            .stride = 0, // changed shortly
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX // TODO: let it be sumn else.
        };

        for (uint32_t v = 0; v < dci.vertexinfocount; ++v){
            if (dci.vertexinfos[v].binding == b){
                bindingdescs[b].stride += DataTypeToSize(dci.vertexinfos[v].type);
            }
        }
    }

    VkVertexInputAttributeDescription attributedescs[dci.vertexinfocount];
    uint32_t lastbinding = 0; uint32_t attributeoffset = 0;
    for (uint32_t a = 0; a < dci.vertexinfocount; ++a){
        if (dci.vertexinfos[a].binding != lastbinding){
            lastbinding = dci.vertexinfos[a].binding;
            attributeoffset = 0;
        }
        attributedescs[a] = (VkVertexInputAttributeDescription){
            .location = dci.vertexinfos[a].location,
            .binding = dci.vertexinfos[a].binding,
            .format = 0, // Changed shortly
            .offset = attributeoffset,
        };
        switch (dci.vertexinfos[a].type){
        case DDE_float1: attributedescs[a].format = VK_FORMAT_R32_SFLOAT; break;
        case DDE_float2: attributedescs[a].format = VK_FORMAT_R32G32_SFLOAT; break;
        case DDE_float3: attributedescs[a].format = VK_FORMAT_R32G32B32_SFLOAT; break;
        case DDE_float4: attributedescs[a].format = VK_FORMAT_R32G32B32A32_SFLOAT; break;
        case DDE_uint1: attributedescs[a].format = VK_FORMAT_R32_UINT; break;
        };
        attributeoffset += DataTypeToSize(dci.vertexinfos[a].type);
    }

    VkPipelineVertexInputStateCreateInfo vici = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = bindingcount + 1,
        .pVertexBindingDescriptions = bindingdescs, 

        .vertexAttributeDescriptionCount = dci.vertexinfocount,
        .pVertexAttributeDescriptions = attributedescs
    };

    VkPipelineInputAssemblyStateCreateInfo iaci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    }; 

    currdraw->vsize = attributeoffset;
    currdraw->voffset = 0;
    totalbuffersize += (dci.vertexcount * currdraw->vsize);

    currdraw->isize = sizeof(uint32_t);
    currdraw->ioffset = currdraw->voffset + totalbuffersize;
    totalbuffersize += (dci.indexcount * currdraw->isize);

    /* VIEW AND SCISSOR */

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)gfx.extent.width,
        .height = (float)gfx.extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = gfx.extent
    };
    VkPipelineViewportStateCreateInfo vpci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    /* RENDERING */

    VkPipelineRasterizationStateCreateInfo rsci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f 
    };

    VkPipelineMultisampleStateCreateInfo msci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE
    };

    VkPipelineDepthStencilStateCreateInfo dsci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_TRUE,
        .front = (VkStencilOpState){
            .failOp = VK_STENCIL_OP_KEEP,
            .passOp = VK_STENCIL_OP_REPLACE,
            .depthFailOp = VK_STENCIL_OP_KEEP,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .compareMask = 0xff,
            .writeMask = 0xff,
            .reference = 1,
        },
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f
    };

    VkPipelineColorBlendAttachmentState cba = {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = (
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT
        )
    };
    if (!dci.transparency){
        cba.blendEnable = VK_FALSE;
    }
    VkPipelineColorBlendStateCreateInfo cbci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_NO_OP,
        .attachmentCount = 1,
        .pAttachments = &cba,
        .blendConstants = {}//{1.0f, 1.0f, 1.0f, 1.0f}
    };


    /* RESOURCES */

    VkDescriptorSetLayoutBinding dslayoutbinding[dci.resourceinfocount];
    uint32_t uniformc = 0, imagec = 0, samplerc = 0;
    /* TODO: dont assume all resources are set 0 */
    for (uint32_t r = 0; r < dci.resourceinfocount; ++r){
        dslayoutbinding[r] = (VkDescriptorSetLayoutBinding){
            .binding = dci.resourceinfos[r].binding,
            .descriptorCount = 1,
            .pImmutableSamplers = 0 /* TODO: dont assume this */
        };
        switch (dci.resourceinfos[r].type){
        case DRTE_uniform:
            dslayoutbinding[r].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            ++uniformc;
            if (dci.resourceinfos[r].uniform.hostvisable){
                *(struct resbuf*)BAGetPointer(currdraw->discoffsets, r)
                    = (struct resbuf){true, totalhcbuffersize, dci.resourceinfos[r].uniform.size };
                totalhcbuffersize += dci.resourceinfos[r].uniform.size;
            }
            else {
                *(struct resbuf*)BAGetPointer(currdraw->discoffsets, r)
                    = (struct resbuf){false, totalbuffersize, dci.resourceinfos[r].uniform.size };
                totalbuffersize += dci.resourceinfos[r].uniform.size;
            }
            break;
        case DRTE_image:
            dslayoutbinding[r].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            ++imagec;
            break;
        case DRTE_sampler:
            dslayoutbinding[r].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            ++samplerc;
            break;
        }
        switch (dci.resourceinfos[r].stage){
        case DRSE_vertex: dslayoutbinding[r].stageFlags = VK_SHADER_STAGE_VERTEX_BIT; break;
        case DRSE_fragment: dslayoutbinding[r].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; break;
        }
    }

    VkDescriptorSetLayoutCreateInfo dslayoutcreateinfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = dci.resourceinfocount,
        .pBindings = dslayoutbinding
    };

    if (vkCreateDescriptorSetLayout(gfx.device, &dslayoutcreateinfo, nullptr, &gfx.drawers[gfx.drawersused-1].disclayout) != VK_SUCCESS){
        ExitOnError("Couldn't create descriptor set layout! (geeb)");
    }

    VkDescriptorPoolSize poolsizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = uniformc
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = samplerc
        }
    };
    VkDescriptorPoolCreateInfo dpci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = 1,
        .poolSizeCount = 2,
        .pPoolSizes = poolsizes,
    };
    if (vkCreateDescriptorPool(gfx.device, &dpci, 0, &gfx.drawers[gfx.drawersused-1].discpool) != VK_SUCCESS){
        ExitOnError("Couldn't create descriptor pool!");
    }
    VkDescriptorSetAllocateInfo sdai = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = 0,
        .descriptorPool = currdraw->discpool,
        .descriptorSetCount = 1,
        .pSetLayouts = &currdraw->disclayout
    };
    if (vkAllocateDescriptorSets( gfx.device, &sdai, &currdraw->discset ) != VK_SUCCESS) {
        ExitOnError("Couldn't allocate descriptor set!\n");
    }

    /* Uniform Buffers */


    CreateBuffer(
        &currdraw->buffers,
        totalbuffersize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );   
    CreateBuffer(
        &currdraw->hcbuffers,
        totalhcbuffersize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    );

    VkDescriptorBufferInfo bufferInfo = {
        .buffer = currdraw->hcbuffers.buffer,
        .offset = 0,
        .range = sizeof(Matrix) * 1024
    };
    VkWriteDescriptorSet descriptorWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = 0,
        .dstSet = currdraw->discset,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pImageInfo = 0,
        .pBufferInfo = &bufferInfo,
        .pTexelBufferView = 0
    };
    vkUpdateDescriptorSets( gfx.device, 1, &descriptorWrite, 0, 0 );

    /* Images */
    DirectImage readimage = directimage_create_bmp("texture_map.bmp");
    uint32_t
        width = readimage.width,
        height = readimage.height;
        
    uint32_t texsize = width * height * 4;

    VkExtent3D sampleriamgeextent = {
        width, height, 1
    };
    VkImageCreateInfo ici = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .extent = sampleriamgeextent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // TODO: not assume
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &gfx.graphics_i,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    vkCreateImage( gfx.device, &ici, 0, &samplers.image);
    VkMemoryRequirements memreq;
    vkGetImageMemoryRequirements( gfx.device, samplers.image, &memreq);

    VkMemoryAllocateInfo mai = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = 0,
        .allocationSize = memreq.size,
        .memoryTypeIndex = FindMemoryType(
            memreq.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        )
    };
    vkAllocateMemory( gfx.device, &mai, 0, &samplers.memory );
    vkBindImageMemory( gfx.device, samplers.image, samplers.memory, 0 );

    VkImageViewCreateInfo ivci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = samplers.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    vkCreateImageView( gfx.device, &ivci, 0, &samplers.imageview );

    GraphicsBuffer interbuffer;
    CreateBuffer(
        &interbuffer,
        texsize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    
    void *texdata;
    vkMapMemory( gfx.device, interbuffer.memory, 0, texsize, 0, &texdata );
        memcpy(texdata, readimage.data, texsize);
    vkUnmapMemory(gfx.device, interbuffer.memory);

    VkCommandBuffer intercommand;
    VkCommandBufferAllocateInfo cbai = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = 0,
        .commandPool = gfx.commandpool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    vkAllocateCommandBuffers( gfx.device, &cbai, &intercommand );
    VkCommandBufferBeginInfo cbbi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = 0,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = 0
    };
    vkBeginCommandBuffer(intercommand, &cbbi);

    VkImageMemoryBarrier imbb = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = 0,
        .srcAccessMask = 0,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, // Ignoring anything on image
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, // Not swapping queue
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, // ownership of img
        .image = samplers.image,
        .subresourceRange = (VkImageSubresourceRange){
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    VkImageMemoryBarrier imba = imbb;
    imba.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imba.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vkCmdPipelineBarrier(
        intercommand,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, 0,
        0, 0,
        1, &imbb
    );
    VkBufferImageCopy bic = {
        .bufferOffset = 0,
        .bufferRowLength = 0,   // ASSUME THE EXTENT
        .bufferImageHeight = 0, // IS THE TEXTURE SIZE
        .imageSubresource = (VkImageSubresourceLayers){
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .imageOffset = 0,
        .imageExtent = (VkExtent3D){
            .width = width,
            .height = height,
            .depth = 1
        }
    };
    vkCmdCopyBufferToImage(
        intercommand,
        interbuffer.buffer,
        samplers.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &bic
    );

    vkCmdPipelineBarrier(
        intercommand,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, 0,
        0, 0,
        1, &imba
    );

    vkEndCommandBuffer(intercommand);
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = 0,
        .pWaitDstStageMask = 0,
        .commandBufferCount = 1,
        .pCommandBuffers = &intercommand,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = 0
    };
    vkQueueSubmit(gfx.graphics, 1, &submitInfo, VK_NULL_HANDLE);

    struct VkSamplerCreateInfo sci2 = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 0.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE
    };
    vkCreateSampler( gfx.device, &sci2, nullptr, &samplers.sampler );
    
    VkDescriptorImageInfo samplerinfo = {
        .sampler = samplers.sampler,
        .imageView = samplers.imageview,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    VkWriteDescriptorSet descriptorwrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = 0,
        .dstSet = currdraw->discset,
        .dstBinding = 1, // CHANGEDDD
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &samplerinfo,
        .pBufferInfo = 0,
        .pTexelBufferView = 0
    };
    // Update the descriptor set to bind the uniform buffer
    vkUpdateDescriptorSets(gfx.device, 1, &descriptorwrite, 0, 0);

    /* Image Samplers */

    VkDescriptorSetLayout dslayouts[] = {
        gfx.drawers[gfx.drawersused-1].disclayout
    };
    VkPushConstantRange pushconstants[] = {
        (VkPushConstantRange){
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = 64 // sizeof matrix (16 * sizeof(float))
        }
    };
    VkPipelineLayoutCreateInfo plci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = dslayouts,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = pushconstants
    };
    vkCreatePipelineLayout( 
        gfx.device, 
        &plci, nullptr, 
        &gfx.drawers[gfx.drawersused-1].pipelinelayout
    );

    /* WRAPPING UP */

    VkGraphicsPipelineCreateInfo pci[1] = {
        {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stageCount = 2, // Vertex & fragment shaders
            .pStages = sci,
            .pVertexInputState = &vici,
            .pInputAssemblyState = &iaci,
            .pTessellationState = nullptr, // OPTIONAL
            .pViewportState = &vpci,
            .pRasterizationState = &rsci,
            .pMultisampleState = &msci,
            .pDepthStencilState = &dsci,
            .pColorBlendState = &cbci,
            .layout = gfx.drawers[gfx.drawersused-1].pipelinelayout,
            .renderPass = gfx.renderpass,
            .subpass = 0
        },
    };

    vkCreateGraphicsPipelines(
        gfx.device, VK_NULL_HANDLE,
        1, pci, nullptr,
        &gfx.drawers[gfx.drawersused-1].pipeline
    );

    return &gfx.drawers[gfx.drawersused-1];
}

uint32_t DataTypeToSize( DrawerDataEnum dde ){
    switch (dde){
        case DDE_float1: return 1 * sizeof(float);
        case DDE_float2: return 2 * sizeof(float);
        case DDE_float3: return 3 * sizeof(float);
        case DDE_float4: return 4 * sizeof(float);
        case DDE_uint1: return 1 * sizeof(uint32_t);
    }
    return 0;
}

void WindowClearScreen( ){

}

void UpdateVertexBuffer( Drawer dr, MeshResource_t t, uint32_t offset ){
    GraphicsBuffer interbuffer;
    CreateBuffer(
        &interbuffer,
        t.vertcount * sizeof(Vertex),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    void* mappeddata;
    vkMapMemory(gfx.device, interbuffer.memory, 0, interbuffer.sizeondevice, 0, &mappeddata);
    memcpy(mappeddata, t.vertdata, interbuffer.sizeondevice);
    vkUnmapMemory(gfx.device, interbuffer.memory);

    VkCommandBuffer intercommand;
    VkCommandBufferAllocateInfo cbai = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = 0,
        .commandPool = gfx.commandpool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    vkAllocateCommandBuffers( gfx.device, &cbai, &intercommand );
    VkCommandBufferBeginInfo cbbi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = 0,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = 0
    };
    vkBeginCommandBuffer(intercommand, &cbbi);

    VkBufferCopy cbcp = {
        0, dr->voffset + offset,
        t.vertcount * sizeof(Vertex),
    };
    vkCmdCopyBuffer(
        intercommand, 
        interbuffer.buffer, dr->buffers.buffer,
        1, &cbcp
    );

    vkEndCommandBuffer(intercommand);

    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = 0,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = 0,
        .pWaitDstStageMask = 0,
        .commandBufferCount = 1,
        .pCommandBuffers = &intercommand,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = 0
    };

    vkQueueSubmit(gfx.graphics, 1, &submit, VK_NULL_HANDLE);
    // vkFreeCommandBuffers( gfx.device, gfx.commandpool, 1, &intercommand );
    vkQueueWaitIdle(gfx.graphics);
    DestroyBuffer( &interbuffer );
}
void UpdateIndexBuffer( Drawer dr, MeshResource_t t, uint32_t offset ){
    GraphicsBuffer interbuffer;
    CreateBuffer(
        &interbuffer,
        t.indcount * sizeof(uint32_t),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    void* mappeddata;
    vkMapMemory(gfx.device, interbuffer.memory, 0, interbuffer.sizeondevice, 0, &mappeddata);
    memcpy(mappeddata, t.inddata, interbuffer.sizeondevice);
    vkUnmapMemory(gfx.device, interbuffer.memory);

    VkCommandBuffer intercommand;
    VkCommandBufferAllocateInfo cbai = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = 0,
        .commandPool = gfx.commandpool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    vkAllocateCommandBuffers( gfx.device, &cbai, &intercommand );
    VkCommandBufferBeginInfo cbbi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = 0,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = 0
    };
    vkBeginCommandBuffer(intercommand, &cbbi);

    VkBufferCopy cbcp = {
        0, dr->ioffset + offset,
        t.indcount * sizeof(uint32_t),
    };
    vkCmdCopyBuffer(
        intercommand, 
        interbuffer.buffer, bufman.ind.buffer,
        1, &cbcp
    );

    vkEndCommandBuffer(intercommand);

    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = 0,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = 0,
        .pWaitDstStageMask = 0,
        .commandBufferCount = 1,
        .pCommandBuffers = &intercommand,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = 0
    };

    vkQueueSubmit(gfx.graphics, 1, &submit, VK_NULL_HANDLE);
    // vkFreeCommandBuffers( gfx.device, gfx.commandpool, 1, &intercommand );
    vkQueueWaitIdle(gfx.graphics);
    DestroyBuffer( &interbuffer );
}
Return_t DrawerUpdateResource( Drawer dr, uint32_t resid, Data_t data, uint32_t size, uint32_t offset ){
    struct resbuf *resbufptr = BAGetPointer(dr->discoffsets, resid);
    if (!resbufptr) return "Resource does not exist";
    if (resbufptr->hc){
        void* mappeddata;
        vkMapMemory( gfx.device, dr->hcbuffers.memory, resbufptr->offset, resbufptr->size, 0, &mappeddata);
        void *mapto = mappeddata + offset;
        memcpy(mapto, data, size);
        vkUnmapMemory(gfx.device, dr->hcbuffers.memory);
    }
        
    return 0;
}

Drawable CreateDrawable( Drawer drawer, DrawableCreateInfo rdi ){
    bufman.memupdated = true;
    MeshMemory *vmem = &bufman.vtxmem;
    bool foundslot = false;
    uint32_t voffset = 0;
    Drawable dr = malloc(sizeof(DrawableDef));
    
    for (uint32_t i = 0; i < rdi.mesh.vertcount; ++i){
        rdi.mesh.vertdata[i].pos = Vector3Transform(
            rdi.mesh.vertdata[i].pos, rdi.transform
        );
    }
    
    dr->ref = ECS_AddEntity( bufman.refs );
    if (dr->ref == UINT32_MAX){
        printf("Ran out of drawable references.....\n");
        exit(-1);
    }
    ECS_AddComp( bufman.refs, dr->ref, ECS_GetComp( bufman.refs, "t" ), 0 );
    ECS_AddComp( bufman.refs, dr->ref, ECS_GetComp( bufman.refs, "m" ), 0 );
    
    *(uint32_t *)ECS_Get( bufman.refs, dr->ref, ECS_GetComp( bufman.refs, "t" ) ) = rdi.mesh.vertdata[0].trsid;
    *(uint32_t *)ECS_Get( bufman.refs, dr->ref, ECS_GetComp( bufman.refs, "m" ) ) = rdi.mesh.vertdata[0].matid;
    
    while (!foundslot){
        if (vmem->next == 0){
            if (vmem->end + (rdi.mesh.vertcount * sizeof(Vertex)) > bufman.vtx.sizeofdata){
                printf("RAN OUT OF DATA RAAAAH\n");
                exit(-1);
            }

            voffset = vmem->end+1;
            vmem->next = malloc( sizeof(MeshMemory) );
            *vmem->next = (MeshMemory){
                voffset, voffset + (rdi.mesh.vertcount * sizeof(Vertex)-1),
                malloc(sizeof(uint32_t) * rdi.mesh.indcount), rdi.mesh.indcount,
                vmem, 0, 
                true, dr->ref
            };
            memcpy(vmem->next->inds, rdi.mesh.inddata, sizeof(uint32_t) * rdi.mesh.indcount);
            foundslot = true;
            // printf("%u <> %u\n", vmem->next->start, vmem->next->end);
        }
        else {
            vmem = vmem->next;
        }
    }
    UpdateVertexBuffer( drawer, rdi.mesh, voffset );

    free( rdi.mesh.vertdata );
    free( rdi.mesh.inddata );

    dr->drawer = drawer;
    return dr;
}
Return_t DrawableSetVisability( Drawable dr, bool vis ){
    if (!ECS_Exists( bufman.refs, dr->ref )){
        return "Attempted to modify a non-existant drawable";
    }
    bufman.memupdated = true;
    MeshMemory *mem = bufman.vtxmem.next;
    while (mem){
        if (mem->dr.ref == dr->ref){
            mem->visable = vis;
        }
        mem = mem->next;
    }
    return 0;
}
Return_t DrawableSetTransform( Drawable dr, Matrix m ){
    if (!ECS_Exists( bufman.refs, dr->ref )){
        return "Attempted to modify a non-existant drawable";
    }

    // Every drawable reference has a trsid and matid
    // TODO: Implement ECS variant where all entities have component enabled by default

    uint32_t trsid = *(uint32_t *)ECS_Get( bufman.refs, dr->ref, ECS_GetComp( bufman.refs, "t" ) );

    return DrawerUpdateResource( dr->drawer, 0, &m, sizeof(Matrix), trsid * sizeof(Matrix) );
}


void RegenerateIndicies( Drawer drawer ){
    MeshMemory *mem = &bufman.vtxmem;
    uint32_t buffer[1024];
    uint32_t intobuffer = 0;
    uint32_t updatedinds = 0;
    while (mem){
        if (!mem->visable){
            mem = mem->next;
            continue;
        }
        uint32_t vert = mem->start / sizeof(Vertex);
        for (uint32_t i = 0; i < mem->indcount; ++i){
            if (intobuffer == 1024){
                MeshResource_t m = {
                    0, 0,
                    buffer, 1024
                };
                UpdateIndexBuffer( drawer, m, updatedinds );
                updatedinds += 1024;
                intobuffer = 0;
            }
            buffer[intobuffer] = mem->inds[i] + vert;
            intobuffer++;
        }
        mem = mem->next;
    }
    MeshResource_t m = {
        0, 0,
        buffer, intobuffer
    };
    UpdateIndexBuffer( drawer, m, updatedinds );
}

void StartRenderPass(uint32_t image){
    //vkResetCommandPool(gfx.device, gfx.commandpool, 0);
    VkCommandBufferBeginInfo cbbi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = 0,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = 0
    };
    vkBeginCommandBuffer( gfx.drawcommand, &cbbi );

    VkClearColorValue colorclear = {
        {0.0f, 0.0f, 0.0f, 1.0f}
    };
    VkClearDepthStencilValue depthstencilclear = {
        1.0, 0
    };
    VkClearValue clearVal[2] = {
        {.color = colorclear},
        {.depthStencil = depthstencilclear}
    };
    VkRenderPassBeginInfo rpbi = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = 0,
        .renderPass = gfx.renderpass,
        .framebuffer = gfx.frames[image],
        .renderArea = {
            .offset = {0, 0},
            .extent = gfx.extent
        },
        .clearValueCount = 2,
        .pClearValues = clearVal
    };
    vkCmdBeginRenderPass( gfx.drawcommand, &rpbi, VK_SUBPASS_CONTENTS_INLINE );
}
void EndRenderPass( ){
    vkCmdEndRenderPass(gfx.drawcommand);
    vkEndCommandBuffer(gfx.drawcommand);
}

void WindowDraw( Drawer drawer, Drawable drawable ){
    if (bufman.memupdated){
        bufman.memupdated = false;
        RegenerateIndicies( drawer );
    }

    vkWaitForFences( gfx.device, 1, &gfx.feninflight, VK_TRUE, UINT64_MAX );
    vkResetFences( gfx.device, 1, &gfx.feninflight );

    uint32_t imageindex;
    VkResult r =
    vkAcquireNextImageKHR( gfx.device, gfx.swapchain, UINT64_MAX, gfx.semimagegrabbed, VK_NULL_HANDLE, &imageindex );

    if (r != 0){
        printf("Failure grabbing image... fuck %d\n", r);
        ExitOnError("Exiting...");
    }

    StartRenderPass( imageindex );
    vkCmdBindPipeline( gfx.drawcommand, VK_PIPELINE_BIND_POINT_GRAPHICS, drawer->pipeline );

    VkBuffer buffers[] = { drawer->buffers.buffer };
    VkDeviceSize offsets[] = { drawer->voffset };
    // TODO: fuuuuuck no okay i need seperate buffers for the mf
    vkCmdBindVertexBuffers(gfx.drawcommand, 0, 1, buffers, offsets);

    vkCmdBindIndexBuffer( gfx.drawcommand, drawer->buffers.buffer, drawer->ioffset, VK_INDEX_TYPE_UINT32 );

    VkDescriptorSet sets[] = {
        drawer->discset
    };
    vkCmdBindDescriptorSets(
        gfx.drawcommand,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        drawer->pipelinelayout,
        0,
        1, sets,
        0, 0
    );

    Matrix m;
    float near = 0.1, far = 5000.0;

    Vector3 lookingat = Vector3Add(
        gfx.cam.position,
        camera_get_forward_XYZ( gfx.cam )
    );
    m = MatrixMultiply(
        MatrixLookAt(
            gfx.cam.position,
            lookingat,
            (Vector3){0, 1, 0}
        ),
        MatrixPerspective(gfx.cam.fov, gfx.cam.aspect, near, far)
    );
    vkCmdPushConstants( gfx.drawcommand, drawer->pipelinelayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 64, &m);
    vkCmdDrawIndexed( gfx.drawcommand, bufman.ind.sizeofdata / sizeof(uint32_t), 1, 0, 0, 0 );

    "eva is so beautiful <3";

    EndRenderPass( );

    VkSemaphore waitSemaphores[] = {gfx.semimagegrabbed};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {gfx.semrenderdone};
    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = 0,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &gfx.drawcommand,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores
    };
    r = vkQueueSubmit( gfx.graphics, 1, &submit, gfx.feninflight );
    if (r != 0){
        printf("Failure Submitting draw command... fuck %d\n", r);
    }

    VkSwapchainKHR swapchains[] = {gfx.swapchain};
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = 0,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &imageindex,
    };

    if (vkQueuePresentKHR( gfx.present, &presentInfo ) != VK_SUCCESS){
        printf("Couldn't Present image :/\n");
        exit(-1);
    }
    vkQueueWaitIdle(gfx.present);
}

void camera_set_main_camera( CameraInfo cam ){
    gfx.cam = cam;
}
CameraInfo camera_get_main_camera( ){
    return gfx.cam;
}
Vector3 camera_get_forward_XZ( CameraInfo cam ){
    float xaxis = sinf(gfx.cam.yaw);
    float zaxis = -cosf(gfx.cam.yaw);

    return (Vector3){xaxis, 0, zaxis};
}
Vector3 camera_get_forward_XYZ( CameraInfo cam ){
    float xaxis = sinf(gfx.cam.yaw) * cosf(gfx.cam.pitch);
    float yaxis = cosf(gfx.cam.pitch + (PI/2.0));
    float zaxis = -cosf(gfx.cam.yaw) * cosf(gfx.cam.pitch);

    return (Vector3){xaxis, yaxis, zaxis};
}

/* UI & MISC */

typedef struct UiComponent_def {
    const char *name;
    uint32_t    type;
    UiConfigs  cnf;

    struct UiComponent_def *parent;
} UiComponent_def;
static void ui_expand_if_needed( UiComponent comp ){
    if (!comp->cnf.usingchildren)
        return;
    /* Has children */
    if ((comp->cnf.children.num+1) < comp->cnf.children.max)
        return;
    /* There isn't enough room */
    uint32_t curmax = comp->cnf.children.max;
    uint32_t newmax = curmax + (curmax / 2);
    comp->cnf.children.them = realloc(
        comp->cnf.children.them,
        sizeof (UiComponent_def) * newmax
    );
}
UiComponent ui_create_root( ){
    UiComponent ret = malloc(sizeof(UiComponent_def));

    ret->type = UI_TYPE_ROOT;
    ret->cnf = (UiConfigs){};
    ret->cnf.usingchildren = true;

    ret->parent = 0;

    UiChildren *kids = &ret->cnf.children;
    kids->max = 16;
    kids->them = (UiComponent_def *)calloc( kids->max, sizeof(UiComponent_def) );
    kids->num = 0;

    return ret;
}
UiComponent ui_create_text( UiComponent parent, const char *name, uint32_t size ){
    if (!parent->cnf.usingchildren)
        return 0;
    ui_expand_if_needed( parent );
    UiComponent ret = parent->cnf.children.them + parent->cnf.children.num;

    return ret;
}
UiComponent ui_create_shape( UiComponent parent, const char *name ){
    return (UiComponent){};
}
void ui_destroy( UiComponent comp ){

}

void ui_set_configs( UiComponent comp, UiConfigs cnf ){

}
UiConfigs ui_get_configs( UiComponent comp ){
    return (UiConfigs){};
}

void ui_generate( UiComponent comp ){

}
void ui_draw( UiComponent comp ){

}

MeshResource_t Mesh_CreateQuad( Matrix m, Box2D tex, uint32_t trsid, uint32_t matid ){
    MeshResource_t ret;

    ret.vertdata = malloc(sizeof(Vertex) * 4);
    ret.vertdata[0] = (Vertex){{-0.5, -0.5, 0.}, {0., 0., 0.}, Box2DGetCorner(tex, 0), trsid, matid}; // TL
    ret.vertdata[1] = (Vertex){{ 0.5, -0.5, 0.}, {0., 0., 0.}, Box2DGetCorner(tex, 1), trsid, matid}; // TR
    ret.vertdata[2] = (Vertex){{-0.5,  0.5, 0.}, {0., 0., 0.}, Box2DGetCorner(tex, 2), trsid, matid}; // BL
    ret.vertdata[3] = (Vertex){{ 0.5,  0.5, 0.}, {0., 0., 0.}, Box2DGetCorner(tex, 3), trsid, matid}; // BR
    ret.vertcount = 4;

    ret.inddata = malloc(sizeof(uint32_t) * 6);
    ret.inddata[0] = 0;
    ret.inddata[1] = 1;
    ret.inddata[2] = 2;
    ret.inddata[3] = 1;
    ret.inddata[4] = 3;
    ret.inddata[5] = 2;
    ret.indcount = 6;

    return ret;
}
