#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform int fps;
uniform vec3 textColor;

// Anti-aliased segment function
float segment(vec2 p, vec2 size) {
    vec2 d = abs(p) - size;
    float dist = max(d.x, d.y);
    return smoothstep(0.02, 0.0, dist);
}

// 7-segment display logic
float digit(int n, vec2 p) {
    float s = 0.0;
    // Segments: a(top), b(top-right), c(bottom-right), d(bottom), e(bottom-left), f(top-left), g(middle)
    bool a=false, b=false, c=false, d=false, e=false, f=false, g=false;
    if(n==0) { a=b=c=d=e=f=true; }
    else if(n==1) { b=c=true; }
    else if(n==2) { a=b=g=e=d=true; }
    else if(n==3) { a=b=g=c=d=true; }
    else if(n==4) { f=g=b=c=true; }
    else if(n==5) { a=f=g=c=d=true; }
    else if(n==6) { a=f=g=e=c=d=true; }
    else if(n==7) { a=b=c=true; }
    else if(n==8) { a=b=c=d=e=f=g=true; }
    else if(n==9) { a=b=c=d=f=g=true; }

    float w = 0.25; // segment width
    float h = 0.05; // segment thickness
    if(a) s = max(s, segment(p - vec2(0.0, 0.5), vec2(w, h)));
    if(b) s = max(s, segment(p - vec2(0.3, 0.25), vec2(h, w)));
    if(c) s = max(s, segment(p - vec2(0.3, -0.25), vec2(h, w)));
    if(d) s = max(s, segment(p - vec2(0.0, -0.5), vec2(w, h)));
    if(e) s = max(s, segment(p - vec2(-0.3, -0.25), vec2(h, w)));
    if(f) s = max(s, segment(p - vec2(-0.3, 0.25), vec2(h, w)));
    if(g) s = max(s, segment(p - vec2(0.0, 0.0), vec2(w, h)));
    return s;
}

// Symbol rendering (F, P, S, :)
float symbol(int ch, vec2 p) {
    float s = 0.0;
    float w = 0.25;
    float h = 0.05;
    if(ch == 70) { // F
        s = max(s, segment(p - vec2(0.0, 0.5), vec2(w, h)));
        s = max(s, segment(p - vec2(0.0, 0.0), vec2(w, h)));
        s = max(s, segment(p - vec2(-0.3, 0.0), vec2(h, 0.5)));
    } else if(ch == 80) { // P
        s = max(s, segment(p - vec2(0.0, 0.5), vec2(w, h)));
        s = max(s, segment(p - vec2(0.0, 0.0), vec2(w, h)));
        s = max(s, segment(p - vec2(-0.3, 0.0), vec2(h, 0.5)));
        s = max(s, segment(p - vec2(0.3, 0.25), vec2(h, 0.25)));
    } else if(ch == 83) { // S
        s = max(s, segment(p - vec2(0.0, 0.5), vec2(w, h)));
        s = max(s, segment(p - vec2(0.0, 0.0), vec2(w, h)));
        s = max(s, segment(p - vec2(0.0, -0.5), vec2(w, h)));
        s = max(s, segment(p - vec2(-0.3, 0.25), vec2(h, 0.25)));
        s = max(s, segment(p - vec2(0.3, -0.25), vec2(h, 0.25)));
    } else if(ch == 58) { // :
        s = max(s, segment(p - vec2(0.0, 0.2), vec2(h, h)));
        s = max(s, segment(p - vec2(0.0, -0.2), vec2(h, h)));
    }
    return s;
}

void main()
{
    float numChars = 8.0;
    float charIdx = floor(TexCoords.x * numChars);
    vec2 p = fract(TexCoords * vec4(numChars, 1.0, 0.0, 0.0).xy);
    p.y = 1.0 - p.y; // Invert Y to fix mirroring
    p = p * 2.0 - 1.0;
    p.x *= 0.6; // fix aspect

    float intensity = 0.0;
    int idx = int(charIdx);
    
    if(idx == 0) intensity = symbol(70, p); // F
    else if(idx == 1) intensity = symbol(80, p); // P
    else if(idx == 2) intensity = symbol(83, p); // S
    else if(idx == 3) intensity = symbol(58, p); // :
    else if(idx == 5) { // Hundreds
        int val = fps / 100;
        if(val > 0) intensity = digit(val, p);
    }
    else if(idx == 6) { // Tens
        int val = (fps / 10) % 10;
        if(fps >= 10) intensity = digit(val, p);
        else if(fps < 10 && val == 0) intensity = 0.0;
    }
    else if(idx == 7) { // Ones
        intensity = digit(fps % 10, p);
    }

    if(intensity > 0.1) {
        FragColor = vec4(textColor, intensity);
    } else {
        discard;
    }
}
