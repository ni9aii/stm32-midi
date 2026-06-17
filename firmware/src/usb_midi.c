#include "usb_midi.h"
#include "midi_packet.h"

#include <libopencm3/usb/audio.h>
#include <libopencm3/usb/midi.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/usbstd.h>
#include <string.h>

#define USB_MIDI_VENDOR_ID 0x1209u
#define USB_MIDI_PRODUCT_ID 0x0001u
#define USB_MIDI_BCD_DEVICE 0x0100u
#define USB_MIDI_MAX_PACKET_SIZE_0 64u
#define USB_MIDI_EP_SIZE 64u
#define USB_MIDI_EP_IN 0x81u
#define USB_MIDI_CONTROL_BUFFER_SIZE 64u
#define USB_MIDI_QUEUE_LEN 32u

static const struct usb_device_descriptor dev_desc = {
    .bLength = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = USB_MIDI_MAX_PACKET_SIZE_0,
    .idVendor = USB_MIDI_VENDOR_ID,
    .idProduct = USB_MIDI_PRODUCT_ID,
    .bcdDevice = USB_MIDI_BCD_DEVICE,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

static const struct usb_audio_header_descriptor_head ac_header = {
    .bLength = sizeof(struct usb_audio_header_descriptor_head),
    .bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
    .bDescriptorSubtype = USB_AUDIO_TYPE_HEADER,
    .bcdADC = 0x0100,
    .wTotalLength = sizeof(struct usb_audio_header_descriptor_head),
    .binCollection = 0,
};

static const struct usb_midi_header_descriptor ms_header = {
    .bLength = sizeof(struct usb_midi_header_descriptor),
    .bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
    .bDescriptorSubtype = USB_MIDI_SUBTYPE_MS_HEADER,
    .bcdMSC = 0x0100,
    .wTotalLength = sizeof(struct usb_midi_header_descriptor) +
                    sizeof(struct usb_midi_in_jack_descriptor) +
                    sizeof(struct usb_midi_out_jack_descriptor) +
                    sizeof(struct usb_midi_endpoint_descriptor),
};

static const struct usb_midi_in_jack_descriptor embedded_in_jack = {
    .bLength = sizeof(struct usb_midi_in_jack_descriptor),
    .bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
    .bDescriptorSubtype = USB_MIDI_SUBTYPE_MIDI_IN_JACK,
    .bJackType = USB_MIDI_JACK_TYPE_EMBEDDED,
    .bJackID = 1,
    .iJack = 0,
};

static const struct usb_midi_out_jack_descriptor embedded_out_jack = {
    .head =
        {
            .bLength = sizeof(struct usb_midi_out_jack_descriptor),
            .bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
            .bDescriptorSubtype = USB_MIDI_SUBTYPE_MIDI_OUT_JACK,
            .bJackType = USB_MIDI_JACK_TYPE_EMBEDDED,
            .bJackID = 2,
            .bNrInputPins = 1,
        },
    .source =
        {
            {
                .baSourceID = 1,
                .baSourcePin = 1,
            },
        },
    .tail =
        {
            .iJack = 0,
        },
};

static const struct usb_midi_endpoint_descriptor midi_endpoint_extra = {
    .head =
        {
            .bLength = sizeof(struct usb_midi_endpoint_descriptor),
            .bDescriptorType = USB_AUDIO_DT_CS_ENDPOINT,
            .bDescriptorSubType = USB_MIDI_SUBTYPE_MS_GENERAL,
            .bNumEmbMIDIJack = 1,
        },
    .jack =
        {
            {
                .baAssocJackID = 1,
            },
        },
};

static const struct __attribute__((packed)) {
  struct usb_midi_header_descriptor header;
  struct usb_midi_in_jack_descriptor in_jack;
  struct usb_midi_out_jack_descriptor out_jack;
} midi_class_descriptors = {
    .header = ms_header,
    .in_jack = embedded_in_jack,
    .out_jack = embedded_out_jack,
};

static const struct usb_endpoint_descriptor midi_endpoint = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_MIDI_EP_IN,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = USB_MIDI_EP_SIZE,
    .bInterval = 1,
    .extra = &midi_endpoint_extra,
    .extralen = sizeof(midi_endpoint_extra),
};

static const struct usb_interface_descriptor ac_iface = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 0,
    .bInterfaceClass = USB_CLASS_AUDIO,
    .bInterfaceSubClass = USB_AUDIO_SUBCLASS_CONTROL,
    .bInterfaceProtocol = 0,
    .iInterface = 0,
    .extra = &ac_header,
    .extralen = sizeof(ac_header),
};

static const struct usb_interface_descriptor midi_iface = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 1,
    .bAlternateSetting = 0,
    .bNumEndpoints = 1,
    .bInterfaceClass = USB_CLASS_AUDIO,
    .bInterfaceSubClass = USB_AUDIO_SUBCLASS_MIDISTREAMING,
    .bInterfaceProtocol = 0,
    .iInterface = 0,
    .endpoint = &midi_endpoint,
    .extra = &midi_class_descriptors,
    .extralen = sizeof(midi_class_descriptors),
};

static const struct usb_interface ifaces[] = {
    {
        .num_altsetting = 1,
        .altsetting = &ac_iface,
    },
    {
        .num_altsetting = 1,
        .altsetting = &midi_iface,
    },
};

static const struct usb_config_descriptor config_desc = {
    .bLength = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType = USB_DT_CONFIGURATION,
    .wTotalLength = 0,
    .bNumInterfaces = 2,
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .bmAttributes = USB_CONFIG_ATTR_DEFAULT,
    .bMaxPower = 50,
    .interface = ifaces,
};

static const struct usb_config_descriptor configs[] = {
    config_desc,
};

static const char *const usb_strings[] = {
    "ni9aii",
    "STM32 MIDI Keyboard",
    "STM32-MIDI-001",
};

static uint8_t usbd_control_buffer[USB_MIDI_CONTROL_BUFFER_SIZE];
static usbd_device *usb_dev;
static bool configured;
static midi_packet_t queue[USB_MIDI_QUEUE_LEN];
static uint8_t queue_head;
static uint8_t queue_tail;
static uint8_t queue_count;

static void reset_queue(void) {
  queue_head = 0;
  queue_tail = 0;
  queue_count = 0;
}

static void set_config(usbd_device *usbd_dev, uint16_t wValue) {
  (void)wValue;

  usbd_ep_setup(usbd_dev, USB_MIDI_EP_IN, USB_ENDPOINT_ATTR_BULK,
                USB_MIDI_EP_SIZE, NULL);
  configured = true;
  reset_queue();
}

static bool queue_event(const midi_packet_t *event) {
  if (!configured || (queue_count >= USB_MIDI_QUEUE_LEN)) {
    return false;
  }

  queue[queue_tail] = *event;
  queue_tail = (queue_tail + 1u) & (USB_MIDI_QUEUE_LEN - 1u);
  queue_count++;

  return true;
}

static void flush_queue(void) {
  if (!configured || (usb_dev == NULL)) {
    return;
  }

  while (queue_count > 0) {
    const midi_packet_t event = queue[queue_head];
    const uint16_t written = usbd_ep_write_packet(
        usb_dev, USB_MIDI_EP_IN, event.bytes, sizeof(event.bytes));

    if (written != sizeof(event.bytes)) {
      break;
    }

    queue_head = (queue_head + 1u) & (USB_MIDI_QUEUE_LEN - 1u);
    queue_count--;
  }
}

void usb_midi_init(void) {
  reset_queue();
  configured = false;

  usb_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev_desc, configs, usb_strings,
                      sizeof(usb_strings) / sizeof(usb_strings[0]),
                      usbd_control_buffer, sizeof(usbd_control_buffer));

  usbd_register_set_config_callback(usb_dev, set_config);
}

bool usb_midi_connected(void) { return configured; }

void usb_midi_poll(void) {
  if (usb_dev != NULL) {
    usbd_poll(usb_dev);
  }

  flush_queue();
}

bool usb_midi_note_on(uint8_t note, uint8_t velocity) {
  const midi_packet_t packet = midi_packet_note_on(note, velocity);

  return queue_event(&packet);
}

bool usb_midi_note_off(uint8_t note, uint8_t velocity) {
  const midi_packet_t packet = midi_packet_note_off(note, velocity);

  return queue_event(&packet);
}
