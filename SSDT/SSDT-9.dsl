/*
 * Intel ACPI Component Architecture
 * AML Disassembler version 20100331
 *
 * Disassembly of SSDT-9.aml, Sun Feb  9 21:47:54 2014
 *
 *
 * Original Table Header:
 *     Signature        "SSDT"
 *     Length           0x00000158 (344)
 *     Revision         0x02
 *     Checksum         0x61
 *     OEM ID           "DELL "
 *     OEM Table ID     "KbbcDevc"
 *     OEM Revision     0x00001000 (4096)
 *     Compiler ID      "INTL"
 *     Compiler Version 0x20130725 (538117925)
 */

DefinitionBlock ("SSDT-9.aml", "SSDT", 2, "DELL ", "KbbcDevc", 0x00001000)
{
    External (\_SB_.PCI0.LPCB, DeviceObj)
    External (\_SB_.PCI0.LPCB.EC0_.KBTL, IntObj) // Keyboard Backlight Time-out Level
    External (\_SB_.PCI0.LPCB.EC0_.KBBL, IntObj) // Keyboard Backlight Brightness Level
    External (\_SB_.PCI0.LPCB.EC0_.MUT0)         // EC Mutex (Lock)

    Scope (\_SB.PCI0.LPCB)
    {        
        Device (KBBC)
        {
            Name (_HID, EisaId ("KBC0000"))         // ACPI Keyboard Backlight Control Device

            Name (BLVL, 0x01)
            Name (BLTO, 0xB4)                       // Set Backlight timeout to 15 min
            
            // Hours in military (24H) format
            Name (HENA, 0x11)                       // Hour of day to enable backlight  (>17:00)
            Name (HDIS, 0x09)                       // Hour of day to disable backlight (>09:00)
  

            Method (BGET, 0, NotSerialized)         // Backlight Brightness Get
            {
                Acquire (^^EC0.MUT0, 0xFFFF)        // Set EC Lock
                Store (^^EC0.KBBL, Local0)
                Release (^^EC0.MUT0)                // Release EC Lock
                Return (Local0)
            }

            Method (BLON, 0, NotSerialized)         // Backlight Enable
            {
                Acquire (^^EC0.MUT0, 0xFFFF)        // Set EC Lock
                Store (BLVL, ^^EC0.KBBL)            // Set Backlight to On /w desired level
                Store (^^EC0.KBBL, Local0)
                Release (^^EC0.MUT0)                // Release EC Lock
                Return (Local0)
            }
            
            Method (BOFF, 0, NotSerialized)         // Backlight Disable
            {
                Acquire (^^EC0.MUT0, 0xFFFF)        // Set EC Lock
                Store (0x00, ^^EC0.KBBL)            // Set Backlight to Off
                Store (^^EC0.KBBL, Local0)
                Release (^^EC0.MUT0)                // Release EC Lock
                Return (Local0)
            }

            Method (TSET, 0, NotSerialized)         // Backlight Timeout Set
            {
                Acquire (^^EC0.MUT0, 0xFFFF)        // Set EC Lock
                Store (BLTO, ^^EC0.KBTL)            // Set desired backlight timeout
                Store (^^EC0.KBTL, Local0)
                Release (^^EC0.MUT0)                // Release EC Lock
                Return (Local0)
            }
        }
    }
}
    