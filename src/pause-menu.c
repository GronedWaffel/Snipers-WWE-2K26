/* WWE 2K26 PPSA34299 01.000.000: native pause-menu integration prototype.
 * Runs only through the game's own UI callbacks, not a debugger-created thread.
 */
typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned char u8;
#define Q(p,o) (*(u64 *)((u8 *)(p)+(o)))
#define D(p,o) (*(u32 *)((u8 *)(p)+(o)))
#define MOD_EVENT 0x534e0000u
#define REEL_EVENT 0xee0f2110u
#ifndef SCRATCH_BASE
#define SCRATCH_BASE 0x4000ULL
#endif
struct State { u64 magic; volatile u8 *finisher; u64 screen; u64 ticks; u64 selections; u64 changed; u32 refresh; u32 enabled; u32 page; u32 saved_count; u64 header; u8 saved[16][20]; };
static struct State state = {0x3155494557504e53ULL, (void *)(SCRATCH_BASE+0x100),0,0,0,0,1,1};
static const char on[]="INFINITE FINISHERS: ON";
static const char off[]="INFINITE FINISHERS: OFF";
static const char title[]="SNIPERS MODS";
static const char back[]="BACK TO PAUSE MENU";
static volatile u8 *const signature=(void *)(SCRATCH_BASE+0x1900);
static volatile u8 *const speed=(void *)(SCRATCH_BASE+0x1901);
static volatile u8 *const ref_ignore=(void *)(SCRATCH_BASE+0x1902);
static volatile u8 *const stamina=(void *)(SCRATCH_BASE+0x1903);
static volatile u8 *const health=(void *)(SCRATCH_BASE+0x1904);
static volatile u8 *const reversal=(void *)(SCRATCH_BASE+0x1905);
static volatile u8 *const unreversible=(void *)(SCRATCH_BASE+0x1906);
static volatile u8 *const kickout=(void *)(SCRATCH_BASE+0x1907);
static volatile u8 *const unlock=(void *)(SCRATCH_BASE+0x1908);
static const char sigon[]="INFINITE SIGNATURE: ON", sigoff[]="INFINITE SIGNATURE: OFF";
static const char speedon[]="SUPER SPEED: ON (3x)", speedoff[]="SUPER SPEED: OFF";
static const char refon[]="REF IGNORE: ON", refoff[]="REF IGNORE: OFF";
static const char staminaon[]="INFINITE STAMINA: ON", staminaoff[]="INFINITE STAMINA: OFF";
static const char healthon[]="INFINITE HEALTH: ON", healthoff[]="INFINITE HEALTH: OFF";
static const char reverseon[]="PERFECT REVERSAL: ON", reverseoff[]="PERFECT REVERSAL: OFF";
static const char noreverseon[]="CAN'T BE REVERSED: ON", noreverseoff[]="CAN'T BE REVERSED: OFF";
static const char kickon[]="AUTO KICK-OUT: ON", kickoff[]="AUTO KICK-OUT: OFF";
static const char unlockon[]="UNLOCK ALL WRESTLERS: ON", unlockoff[]="UNLOCK ALL WRESTLERS: OFF";
static const char more[]="MORE MODS >", previous[]="< BACK TO MODS";
static void copy(void *dst,const void *src,u32 n) {for(u32 i=0;i<n;i++)((u8*)dst)[i]=((const u8*)src)[i];}
static void redraw(void *screen) {
    if (!screen || Q(screen,0)!=0x4e060f8) return;
    void *list=(void *)Q(screen,0x58); if (!list) return;
    void *header=(void *)Q(list,0x30); if (!header) return;
    u32 n=D(header,0x14); if(n>16 || n>D(list,0x2c)) return;
    u8 *rows=(void *)Q(header,0), *display=(void *)Q(list,0x18);
    if (!rows || !display) return;
    if(state.screen!=(u64)screen || state.header!=(u64)header) {
        state.screen=(u64)screen; state.header=(u64)header; state.page=0;
    }
    if(!state.page) {
        u32 found=0; for(u32 i=0;i<n;i++)if(D(rows+i*0x120,0x10)==MOD_EVENT){found=1;Q(rows+i*0x120,0)=(u64)title;}
        if(!found) {
            if(n>=D(header,0x10)||n>=D(list,0x2c))return;
            copy(rows+n*0x120,rows,0x120);
            Q(rows+n*0x120,0)=(u64)title; D(rows+n*0x120,0x10)=MOD_EVENT;
            D(header,0x14)=++n;
        }
    } else if(state.page==2) {
        if(D(header,0x10)<3 || D(list,0x2c)<3)return;
        Q(rows,0)=(u64)(*kickout?kickon:kickoff);D(rows,0x10)=MOD_EVENT+9;
        Q(rows+0x120,0)=(u64)(*unlock?unlockon:unlockoff);D(rows+0x120,0x10)=MOD_EVENT+10;
        Q(rows+0x240,0)=(u64)previous;D(rows+0x240,0x10)=MOD_EVENT+0xfe;
        D(header,0x14)=3;
    } else {
        if(D(header,0x10)<10 || D(list,0x2c)<10)return;
        Q(rows,0)=(u64)(*state.finisher?on:off);D(rows,0x10)=MOD_EVENT+1;
        Q(rows+0x120,0)=(u64)(*signature?sigon:sigoff);D(rows+0x120,0x10)=MOD_EVENT+2;
        Q(rows+0x240,0)=(u64)(*speed?speedon:speedoff);D(rows+0x240,0x10)=MOD_EVENT+3;
        Q(rows+0x360,0)=(u64)(*ref_ignore?refon:refoff);D(rows+0x360,0x10)=MOD_EVENT+4;
        Q(rows+0x480,0)=(u64)(*stamina?staminaon:staminaoff);D(rows+0x480,0x10)=MOD_EVENT+5;
        Q(rows+0x5a0,0)=(u64)(*health?healthon:healthoff);D(rows+0x5a0,0x10)=MOD_EVENT+6;
        Q(rows+0x6c0,0)=(u64)(*reversal?reverseon:reverseoff);D(rows+0x6c0,0x10)=MOD_EVENT+7;
        Q(rows+0x7e0,0)=(u64)(*unreversible?noreverseon:noreverseoff);D(rows+0x7e0,0x10)=MOD_EVENT+8;
        Q(rows+0x900,0)=(u64)more;D(rows+0x900,0x10)=MOD_EVENT+0xfd;
        Q(rows+0xa20,0)=(u64)back;D(rows+0xa20,0x10)=MOD_EVENT+0xff;
        D(header,0x14)=10;
    }
    ((void (*)(void *))0x1ce1d00)(list);
    state.changed++;state.refresh=0;
}
void menu_tick(void *screen) {
    state.ticks++;
    /* The allocator may reuse a destroyed pause screen at the same address. */
    if(screen && Q(screen,0)==0x4e060f8) {
        void *list=(void *)Q(screen,0x58);
        if(list) {void *header=(void *)Q(list,0x30);
            if(header && Q(header,0) && D(header,0x14)>0 && D(header,0x14)<=16) {
                u8 *rows=(void *)Q(header,0);u32 found=0;
                for(u32 i=0;i<D(header,0x14);i++)if((D(rows+i*0x120,0x10)&0xffff0000u)==MOD_EVENT)found=1;
                if(!found){state.page=0;state.refresh=1;}
            }
        }
    }
    if(state.enabled && (state.refresh || state.screen!=(u64)screen))redraw(screen);
    ((void (*)(void *))0x1cefa30)(screen);
}
u64 menu_select(void *screen,u64 a,u64 b,u64 event) {
    if(state.enabled && ((u32)event&0xffff0000u)==MOD_EVENT && screen && Q(screen,0)==0x4e060f8) {
        void *list=(void *)Q(screen,0x58); if(!list)return 0xffffffff;
        void *header=(void *)Q(list,0x30);if(!header)return 0xffffffff;
        u8 *rows=(void*)Q(header,0);if(!rows)return 0xffffffff;
        if((u32)event==MOD_EVENT && !state.page) {
            u32 n=D(header,0x14);if(n>16)return 0xffffffff;
            for(u32 i=0;i<n;i++)copy(state.saved[i],rows+i*0x120,20);
            state.saved_count=n;state.page=1;D(list,0x38)=0;D(list,0x40)=0xffffffff;D(list,0x68)=0;
        } else if((u32)event==MOD_EVENT+1 && state.page) {*state.finisher=!*state.finisher;}
        else if((u32)event==MOD_EVENT+2 && state.page) {*signature=!*signature;}
        else if((u32)event==MOD_EVENT+3 && state.page) {*speed=!*speed;}
        else if((u32)event==MOD_EVENT+4 && state.page) {*ref_ignore=!*ref_ignore;}
        else if((u32)event==MOD_EVENT+5 && state.page) {*stamina=!*stamina;}
        else if((u32)event==MOD_EVENT+6 && state.page) {*health=!*health;}
        else if((u32)event==MOD_EVENT+7 && state.page) {*reversal=!*reversal;}
        else if((u32)event==MOD_EVENT+8 && state.page) {*unreversible=!*unreversible;}
        else if((u32)event==MOD_EVENT+9 && state.page) {*kickout=!*kickout;}
        else if((u32)event==MOD_EVENT+10 && state.page) {*unlock=!*unlock;}
        else if((u32)event==MOD_EVENT+0xfd && state.page==1) {
            state.page=2;D(list,0x38)=0;D(list,0x40)=0xffffffff;D(list,0x68)=0;
        }
        else if((u32)event==MOD_EVENT+0xfe && state.page==2) {
            state.page=1;D(list,0x38)=0;D(list,0x40)=0xffffffff;D(list,0x68)=0;
        }
        else if((u32)event==MOD_EVENT+0xff && state.page) {
            for(u32 i=0;i<state.saved_count;i++)copy(rows+i*0x120,state.saved[i],20);
            D(header,0x14)=state.saved_count;state.page=0;D(list,0x38)=0;D(list,0x40)=0xffffffff;D(list,0x68)=0;
        }
        state.selections++; state.refresh=1;
        return 0xffffffff;
    }
    return ((u64 (*)(void *,u64,u64,u64))0x1d88280)(screen,a,b,event);
}
u8 menu_cancel(void *screen,void *input) {
    if(state.enabled && state.page && (u64)screen==state.screen) {
        menu_select(screen,0,0,MOD_EVENT+(state.page==2?0xfe:0xff));
        return 1; /* Consume Circle; don't call the native resume action. */
    }
    return ((u8 (*)(void *,void *))0x1cf0360)(screen,input);
}
float speed_hook(void *locomotion) {
    float value=((float (*)(void *))(SCRATCH_BASE+0x1b00))(locomotion);
    if(*speed && locomotion) {
        void *owner=(void *)Q(locomotion,0x88);
        if(owner) {void *controller=(void *)Q(owner,0x25f8);
            if(controller && !*((u8*)controller+0xca))value*=3.0f;
        }
    }
    return value;
}
u64 referee_dq(void *transition,void *referee) {
    if(*ref_ignore)return 0;
    return ((u64(*)(void *,void *))0x13c6ef0)(transition,referee);
}
void stamina_hook(void *meter,float value) {
    if(*stamina && meter) {
        void *owner=(void *)Q(meter,8);
        if(owner) {
            void *controller=(void *)Q(owner,0x25f8), *components=(void *)Q(owner,0x25c8);
            if(controller && components && !*((u8*)controller+0xca) && Q(components,0x308)==(u64)meter) {
                float maximum=((float (*)(void *))0x1094390)(meter);
                if(maximum>0.0f && maximum<100000.0f)value=maximum;
            }
        }
    }
    ((void (*)(void *,float))(SCRATCH_BASE+0x1b20))(meter,value);
}
static float health_value(void *meter,float value) {
    if(*health && meter) {
        void *owner=(void *)Q(meter,0);
        if(owner) {
            void *controller=(void *)Q(owner,0x25f8), *components=(void *)Q(owner,0x25c8);
            if(controller && components && !*((u8*)controller+0xca) && Q(components,0x2e0)==(u64)meter)return 100.0f;
        }
    }
    return value;
}
void health_cap(void *meter,float value){((void (*)(void *,float))(SCRATCH_BASE+0x1b40))(meter,health_value(meter,value));}
void health_current(void *meter,float value){((void (*)(void *,float))(SCRATCH_BASE+0x1b60))(meter,health_value(meter,value));}
void health_recovery(void *meter,float value){((void (*)(void *,float))(SCRATCH_BASE+0x1b80))(meter,health_value(meter,value));}
static u8 human(void *owner) {
    if(!owner)return 0;
    u32 slot=D(owner,0xa0);
    if(slot>=255 || ((u64*)0x5352548)[slot]!=(u64)owner)return 0;
    void *controller=(void *)Q(owner,0x25f8);
    return controller && !*((u8*)controller+0xca);
}
u8 reversal_ability(void *owner,u32 ability) {
    /* Hook the native reversal completion and held-grapple checks.
     * Ability 6 accepts a normal/major reversal; 11 is the held-grapple
     * counter used by grab/Irish-whip transitions; 12 is a combo breaker. */
    if(*reversal && human(owner) && (ability==6 || ability==11 || ability==12)) {
        (*(volatile u64*)(SCRATCH_BASE+(ability==6?0x1948:ability==11?0x1990:0x1950)))++;
        return 1;
    }
    return ((u8 (*)(void *,u32))0x10704b0)(owner,ability);
}
static u8 block_counter(void *defender) {
    if(!*unreversible || !defender || human(defender))return 0;
    void *attacker=(void*)Q(defender,0xf70);
    if(attacker && human(attacker)) {
        (*(volatile u64*)(SCRATCH_BASE+0x1958))++;
        return 1;
    }
    return 0;
}
u64 complete_normal(u64 unused,void *defender){if(block_counter(defender))return 0;return ((u64 (*)(u64,void *))0x117cea0)(unused,defender);}
u64 complete_major(u64 unused,void *defender){if(block_counter(defender))return 0;return ((u64 (*)(u64,void *))0x117d020)(unused,defender);}
u64 complete_breaker(u64 unused,void *defender){if(block_counter(defender))return 0;return ((u64 (*)(u64,void *))0x117d150)(unused,defender);}
u8 breaker_input(void *buffer) {
    /* Only the eligibility call in 117d070 is redirected. Its early input
     * buffer check otherwise prevents the native ability-12 auto breaker. */
    if(*reversal && buffer && human((u8*)buffer-0xfb8)) {
        (*(volatile u64*)(SCRATCH_BASE+0x1960))++;
        return 1;
    }
    return ((u8 (*)(void *))0x1563de0)(buffer);
}
u8 breaker_gate(void *behavior,void *defender,u32 force) {
    u8 result=((u8 (*)(void *,void *,u32))0x10561a0)(behavior,defender,force);
    if(*reversal && human(defender)) {
        (*(volatile u64*)(SCRATCH_BASE+0x1968))++;
        if(result)(*(volatile u64*)(SCRATCH_BASE+0x1970))++;
        D((void*)(SCRATCH_BASE+0x1978),0)=D(behavior,0xc4);
    }
    return result;
}
void pin_action(void *controller) {
    if(*kickout && controller && D(controller,8)==3 && human((void*)Q(controller,0x18))) {
        u64 vt=Q(controller,0);
        if(vt==0x4cd0be8 || vt==0x4cd0cd0 || vt==0x4cd0e00 || vt==0x4cd0f30) {
            /* Use the derived success transition: it updates the state and
             * publishes the native HUD event, including timed-pin cleanup. */
            ((void (*)(void *))Q((void*)vt,0xa0))(controller);
            (*(volatile u64*)(SCRATCH_BASE+0x1980))++;
        }
    }
    ((void (*)(void *))0x134a590)(controller);
}
u8 roster_unlocked(u64 id) {
    if(*unlock) {
        void *item=((void *(*)(u64))0x809ea0)(id);
        /* Catalog type 1 is a wrestler; 16 is a wrestler's alternate attire.
         * This overrides a query only, without writing the save or catalog. */
        if(item && (*((u8*)item+0x89)==1 || *((u8*)item+0x89)==16)) {
            (*(volatile u64*)(SCRATCH_BASE+0x1988))++;
            return 1;
        }
    }
    return ((u8 (*)(u64))(SCRATCH_BASE+0x1bc0))(id);
}
u64 referee_count(void *transition,void *referee) {
    if(*ref_ignore)return 0;
    return ((u64(*)(void *,void *))0x1154dd0)(transition,referee);
}
__attribute__((section(".header"),used))
const u64 exports[]={0x3155494557504e53ULL,(u64)menu_tick,(u64)menu_select,(u64)&state,(u64)speed_hook,(u64)referee_dq,(u64)referee_count,(u64)menu_cancel,(u64)stamina_hook,(u64)health_cap,(u64)health_current,(u64)health_recovery,(u64)reversal_ability,(u64)complete_normal,(u64)complete_major,(u64)complete_breaker,(u64)breaker_input,(u64)breaker_gate,(u64)pin_action,(u64)roster_unlocked};
