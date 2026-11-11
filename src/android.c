// Android only (see android.h). The folder chooser and the copying are Java
// (ComboBreakerActivity); this side asks for them through JNI and waits.
#include "android.h"
#include "game.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_system.h>
#include <jni.h>
#include <stdio.h>

// Calls a no-argument method of the activity: "V" void, "I" int (returned).
static int activity_call(const char *name, char type)
{
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject act = (jobject)SDL_AndroidGetActivity();
    if (!env || !act) return -1;
    jclass cls = (*env)->GetObjectClass(env, act);
    jmethodID m = (*env)->GetMethodID(env, cls, name, type == 'I' ? "()I" : "()V");
    int r = -1;
    if (m) {
        if (type == 'I') r = (*env)->CallIntMethod(env, act, m);
        else { (*env)->CallVoidMethod(env, act, m); r = 0; }
    }
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); r = -1; }
    (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, act);
    return r;
}

bool android_pick_game_files(const char *missing)
{
    SDL_InitSubSystem(SDL_INIT_VIDEO);
    char msg[2048];
    snprintf(msg, sizeof msg,
             "%s plays your own copy of the arcade game's files (MAME's \"%s\" set, version L1.5D). "
             "No game files come with the app.\n\n"
             "Choose the folder on your phone that holds them (a MAME %s.zip there works too). "
             "They are copied into the app once.\n\n"
             "Still needed:%s\n\n"
             "Elegí la carpeta del teléfono donde tenés tus archivos del juego.",
             game.product, game.set, game.set, missing);
    const SDL_MessageBoxButtonData buttons[] = {
        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Choose folder" },
        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Quit" },
    };
    const SDL_MessageBoxData box = { SDL_MESSAGEBOX_INFORMATION, NULL, game.product, msg, 2, buttons, NULL };
    int pressed = 0;
    if (SDL_ShowMessageBox(&box, &pressed) != 0 || pressed != 1) return false;
    if (activity_call("pickGameFiles", 'V') < 0) return false;
    // 0 choosing or copying, 1 copied, 2 cancelled, 3 failed
    int st;
    while ((st = activity_call("pickState", 'I')) == 0) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) if (ev.type == SDL_QUIT) return false;
        SDL_Delay(50);
    }
    fprintf(stderr, "android: folder chooser finished (%d)\n", st);
    return st >= 0;
}
