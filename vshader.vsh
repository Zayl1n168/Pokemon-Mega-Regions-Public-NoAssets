; 4x4 Uniform Matrices
.fvec projection[4], modelView[4]

; Constants
.constf myconst(0.0, 1.0, -1.0, 0.5)
.alias  zeros myconst.xxxx ; 0.0
.alias  ones  myconst.yyyy ; 1.0

; Outputs
.out outpos position
.out outtex texcoord0

; Inputs (Hardware attribute registers)
.alias inpos v0 ; Attribute 0: Position (X, Y, Z)
.alias intex v1 ; Attribute 1: UV (U, V)

.proc main
    ; Force input W component to 1.0
    mov r0.xyz, inpos
    mov r0.w,   ones

    ; Calculate ModelView transformation (r1 = modelView * r0)
    dp4 r1.x, modelView[0], r0
    dp4 r1.y, modelView[1], r0
    dp4 r1.z, modelView[2], r0
    dp4 r1.w, modelView[3], r0

    ; Calculate Projection transformation (outpos = projection * r1)
    dp4 outpos.x, projection[0], r1
    dp4 outpos.y, projection[1], r1
    dp4 outpos.z, projection[2], r1
    dp4 outpos.w, projection[3], r1

    ; --- TEXTURE COORDINATE PASS-THROUGH & V-FLIP ---
    mov outtex.x, intex.x
    mov r0, ones
    add outtex.y, r0.y, -intex.y

    end
.end
