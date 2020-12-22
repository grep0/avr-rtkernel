env = Environment()
Export('env')

# Set environment for AVR-GCC.
env['CC'] = 'avr-gcc'
env['CXX'] = 'avr-g++'
env['AS'] = 'avr-gcc'
env['CPPPATH'] = ['/usr/avr/include/', 'build']
env['OBJCOPY'] = 'avr-objcopy'
env['OBJDUMP'] = 'avr-objdump'
env['SIZE'] = 'avr-size'
env['AVRDUDE'] = 'avrdude'
env.Append(CCFLAGS = '-O1 -Wall -flto -g -fno-inline')
env.Append(LINKFLAGS = '-g')

# Declare some variables about microcontroller.
# Microcontroller type.
DEVICE = 'atmega328p'
PROG_TYPE = 'arduino'
PROG_PORT = '/dev/ttyACM0'
PROG_DEVICE = 'm328p'

# Set environment for an Atmel AVR Atmega 328p microcontroller.
# Create and initialize the environment.
env.Append(CCFLAGS = '-mmcu=' + DEVICE)
env.Append(CXXFLAGS = '-Wno-class-memaccess')
env.Append(LINKFLAGS = '-mmcu=' + DEVICE)

# Define target name.
TARGET = 'build/main'

SConscript('src/SConscript', variant_dir='build', duplicate=0)

Default(env.Command(TARGET+'.dump', TARGET+'.elf',
    env['OBJDUMP'] + ' -D $SOURCE > $TARGET'))
env.Command('prog', TARGET+'.elf',
    env['AVRDUDE'] + ' -p '+ PROG_DEVICE + ' -c ' + PROG_TYPE + ' -P ' + PROG_PORT + \
    ' -U flash:w:$TARGET:e')
