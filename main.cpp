#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>       // memcpy()
#include <math.h>

#if 1
#include <iostream>
using namespace std;
#endif // 1

#define MAXWIRES 512


typedef struct
{
    int    x, y, z;
} endp_t;

typedef struct
{
    endp_t ep1, ep2;
} wire_t;

typedef struct
{
    int    n;
    wire_t wire[MAXWIRES];
} wirelist_t;

// Workspace variabler
wirelist_t  rail_PS_upper, rail_STB_upper;   // Rails port side and star board side
wirelist_t  rail_PS_lower, rail_STB_lower;   // Rails port side and star board side
wirelist_t  rail_PS,       rail_STB;

// Result variables
wirelist_t  rails;
wirelist_t  mast;

//------------------------------------------------------------------------------------

void set_wire( wire_t *wire, int x1, int y1, int z1, int x2, int y2, int z2 )
{
    wire->ep1.x = x1;  wire->ep1.y = y1;  wire->ep1.z = z1;
    wire->ep2.x = x2;  wire->ep2.y = y2;  wire->ep2.z = z2;
}


int append_wire( wirelist_t *wirelist, int x2, int y2, int z2 )
{
    wire_t *wire  = &wirelist->wire[ wirelist->n ];
    wire_t *wire0 =  wire - 1;

    set_wire( wire, wire0->ep2.x, wire0->ep2.y, wire0->ep2.z, x2, y2, z2 );
    return ++wirelist->n;
}


int add_wire( wirelist_t *wirelist, endp_t *ep1, endp_t *ep2 )
{
    wire_t *wire  = &wirelist->wire[ wirelist->n ];

    set_wire( wire, ep1->x, ep1->y, ep1->z, ep2->x, ep2->y, ep2->z );
    return ++wirelist->n;
}


void mirrorX_wires( wirelist_t *result, wirelist_t *src )
{
    memcpy( result, src, sizeof(wirelist_t) );

    int  i;
    for (i = 0; i < src->n; i++)
    {
        result->wire[i].ep1.x *= -1;
        result->wire[i].ep2.x *= -1;
    }
}

//---------------------------------------------------------------------------------------------

// Connect lower rail wire to upper wire
void connect_lower2upper( wirelist_t *result, wirelist_t *upper, wirelist_t *lower )
{
    int   i;
    for ( i = 0; i < upper->n; i++ )
    {
        memcpy( result->wire, upper->wire, upper->n * sizeof(wire_t) );
    }
    result->n = upper->n;

    for ( i = 0; i < lower->n; i++ )
    {
        memcpy( &result->wire[result->n], &lower->wire, lower->n * sizeof(wire_t) );
    }
    result->n += lower->n;

    for ( i = 0; i < upper->n; i++ )
    {
        result->n = add_wire( result, &upper->wire[i].ep2, &lower->wire[i].ep2 );
    }
    result->n = add_wire( result, &upper->wire[0].ep1, &lower->wire[0].ep1 );
}


void set_rails( int detailed, int height )
{
    // Port side rail(s)
    set_wire( &rail_PS_upper.wire[0], 70, 0, height,    120,   0, height    );
    set_wire( &rail_PS_lower.wire[0], 70, 0, height-30, 120,   0, height-30 );

    rail_PS_upper.n += 1;
    rail_PS_upper.n  = append_wire( &rail_PS_upper,  130,  30, height );
    rail_PS_upper.n  = append_wire( &rail_PS_upper,  147, 240, height );
    rail_PS_upper.n  = append_wire( &rail_PS_upper,  135, 424, height );
    rail_PS_upper.n  = append_wire( &rail_PS_upper,   90, 614, height );
    rail_PS_upper.n  = append_wire( &rail_PS_upper,   32, 784, height );
    rail_PS_upper.n  = append_wire( &rail_PS_upper,   22, 805, height );

    rail_PS_lower.n += 1;
    rail_PS_lower.n  = append_wire( &rail_PS_lower,  130,  45, height-30 );
    rail_PS_lower.n  = append_wire( &rail_PS_lower,  147, 240, height-30 );
    rail_PS_lower.n  = append_wire( &rail_PS_lower,  135, 424, height-30 );
    rail_PS_lower.n  = append_wire( &rail_PS_lower,   90, 614, height-30 );
    rail_PS_lower.n  = append_wire( &rail_PS_lower,   32, 770, height-30 );
    rail_PS_lower.n  = append_wire( &rail_PS_lower,   22, 815, height-30 );

    if (detailed)
    {
        connect_lower2upper( &rail_PS, &rail_PS_upper, &rail_PS_lower );
    }
    mirrorX_wires( &rail_STB, &rail_PS );

    int  i;
    for (i = 0; i < rail_PS.n; i++ ) {
        rails.wire[rails.n++] = rail_PS.wire[i];
    }
    for (i = 0; i < rail_PS.n; i++ ) {
        rails.wire[rails.n++] = rail_STB.wire[i];
    }
    add_wire( &rails, &rail_PS.wire[0].ep1, &rail_STB.wire[0].ep1 );
}

//--------------------------------------------------------------------------

float scale( int value )
{
    // Scale "cm" -> "m"
    float  result = value;
    return result / 100.0;
}


void export_nec_radiator( float start_freq )
{
    float radiator_len = 9.8;

    printf("CM Add your....\n");
    printf("CM ...comment(s)...\n");
    printf("CM ...here\n");
    printf("CE\n");

    printf("SY freq=7.000\n");
    printf("SY WL=299.8/freq, M=WL/999	'Minimal NEC segment length\n");

    printf("SY len=%06.3f  'Radiator length\n", radiator_len);
    printf("SY fph=1.500   'Feed point height\n");

    printf("GW	1	3	0.700	0	fph	0.700	0	fph+3*M	0.0012941	'Short wire for excitation source\n");
    printf("GW	2	70	0.700	0	fph+3*M	0	2.500	fph+len	0.0012941	'Rest of radiatior\n");
}


// Argument "start_freq" is lowes simulatio frequency
void export_nec( wirelist_t *wirelist, float start_freq )
{
    wire_t *wire = &wirelist->wire[0];

    float rail_thikness = 0.0040;
    float min_seglen    = (299.8/start_freq) / 999.0;

    // Rails
    int  i;
    for( i = 0; i < wirelist->n; i++, wire++ )
    {
        float dx = scale(wire->ep1.x) - scale(wire->ep2.x);
        float dy = scale(wire->ep1.y) - scale(wire->ep2.y);
        float dz = scale(wire->ep1.z) - scale(wire->ep2.z);

        float wirelen  = sqrtf( dx*dx + dy*dy + dz*dz );
        int   segments = wirelen / min_seglen;
        int   tag_id   = 0;

        // Make segment count odd value
        if ((segments &  1) == 0)
             segments -= 1;

        printf("GW\t%d\t%2d\t%5.3f\t%5.3f\t%5.3f\t%5.3f\t%5.3f\t%5.3f\t%7.5f\n",
               tag_id, segments,
               scale(wire->ep1.x), scale(wire->ep1.y), scale(wire->ep1.z),
               scale(wire->ep2.x), scale(wire->ep2.y), scale(wire->ep2.z),
               rail_thikness );
    }
}

//------------------------------------------------------------------------------

int main()
{
    set_rails( 1, 150 );

    export_nec_radiator( 3.000 );
    export_nec( &rails,  3.000 );
    export_nec( &mast,   3.000 );

    return 0;
}
