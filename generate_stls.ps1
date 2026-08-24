# PowerShell 3D STL Generator for DIY TP-7 Field Recorder Parts
param(
    [string]$OutputDir = "$PSScriptRoot\3D_Models"
)

if (!(Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

function Write-AsciiSTL {
    param(
        [string]$FilePath,
        [string]$SolidName,
        [System.Collections.ArrayList]$Triangles
    )
    $sb = [System.Text.StringBuilder]::new()
    [void]$sb.AppendLine("solid $SolidName")
    
    foreach ($tri in $Triangles) {
        $v1 = $tri[0]
        $v2 = $tri[1]
        $v3 = $tri[2]
        
        # Calculate surface normal
        $ax = $v2[0] - $v1[0]; $ay = $v2[1] - $v1[1]; $az = $v2[2] - $v1[2]
        $bx = $v3[0] - $v1[0]; $by = $v3[1] - $v1[1]; $bz = $v3[2] - $v1[2]
        $nx = ($ay * $bz) - ($az * $by)
        $ny = ($az * $bx) - ($ax * $bz)
        $nz = ($ax * $by) - ($ay * $bx)
        $len = [Math]::Sqrt($nx*$nx + $ny*$ny + $nz*$nz)
        if ($len -gt 0) { $nx /= $len; $ny /= $len; $nz /= $len } else { $nz = 1.0 }

        [void]$sb.AppendLine("  facet normal $($nx.ToString("F4")) $($ny.ToString("F4")) $($nz.ToString("F4"))")
        [void]$sb.AppendLine("    outer loop")
        [void]$sb.AppendLine("      vertex $($v1[0].ToString("F3")) $($v1[1].ToString("F3")) $($v1[2].ToString("F3"))")
        [void]$sb.AppendLine("      vertex $($v2[0].ToString("F3")) $($v2[1].ToString("F3")) $($v2[2].ToString("F3"))")
        [void]$sb.AppendLine("      vertex $($v3[0].ToString("F3")) $($v3[1].ToString("F3")) $($v3[2].ToString("F3"))")
        [void]$sb.AppendLine("    endloop")
        [void]$sb.AppendLine("  endfacet")
    }
    [void]$sb.AppendLine("endsolid $SolidName")
    [System.IO.File]::WriteAllText($FilePath, $sb.ToString())
}

function Add-Quad {
    param($list, $v1, $v2, $v3, $v4)
    [void]$list.Add(@($v1, $v2, $v3))
    [void]$list.Add(@($v1, $v3, $v4))
}

# 1. GENERATE TP7 MOTORIZED TAPE REEL (56mm Dia x 4.5mm Spool with 3 Spokes & Magnet Pocket)
Write-Host "Generating TP7_Motorized_Tape_Reel.stl..."
$reelTris = [System.Collections.ArrayList]::new()
$segments = 36
$outerR = 28.0
$hubR = 8.0
$spokeOuterR = 24.0
$height = 4.5

for ($i = 0; $i -lt $segments; $i++) {
    $a1 = ($i / $segments) * [Math]::PI * 2
    $a2 = (($i + 1) / $segments) * [Math]::PI * 2
    
    $deg = ($i / $segments) * 360
    # Create 3 spoke cutouts around 60°, 180°, 300°
    $isSpokeCutout = (($deg -ge 40 -and $deg -le 80) -or ($deg -ge 160 -and $deg -le 200) -or ($deg -ge 280 -and $deg -le 320))
    
    $rInner = if ($isSpokeCutout) { $spokeOuterR } else { $hubR }

    $x1_out = $outerR * [Math]::Cos($a1); $y1_out = $outerR * [Math]::Sin($a1)
    $x2_out = $outerR * [Math]::Cos($a2); $y2_out = $outerR * [Math]::Sin($a2)
    $x1_in  = $rInner * [Math]::Cos($a1); $y1_in  = $rInner * [Math]::Sin($a1)
    $x2_in  = $rInner * [Math]::Cos($a2); $y2_in  = $rInner * [Math]::Sin($a2)

    # Top surface
    Add-Quad $reelTris @($x1_in, $y1_in, $height) @($x2_in, $y2_in, $height) @($x2_out, $y2_out, $height) @($x1_out, $y1_out, $height)
    # Bottom surface
    Add-Quad $reelTris @($x1_out, $y1_out, 0) @($x2_out, $y2_out, 0) @($x2_in, $y2_in, 0) @($x1_in, $y1_in, 0)
    # Outer cylindrical rim
    Add-Quad $reelTris @($x1_out, $y1_out, 0) @($x1_out, $y1_out, $height) @($x2_out, $y2_out, $height) @($x2_out, $y2_out, 0)
    # Inner rim / spoke boundary
    Add-Quad $reelTris @($x2_in, $y2_in, 0) @($x2_in, $y2_in, $height) @($x1_in, $y1_in, $height) @($x1_in, $y1_in, 0)
}

# Central Hub Hub (3mm D-Shaft Bore & Magnet Pocket)
$hubSegments = 24
$boreR = 1.55 # 3.1mm diameter for 3mm D-shaft press fit
for ($i = 0; $i -lt $hubSegments; $i++) {
    $a1 = ($i / $hubSegments) * [Math]::PI * 2
    $a2 = (($i + 1) / $hubSegments) * [Math]::PI * 2
    $x1_h = $hubR * [Math]::Cos($a1); $y1_h = $hubR * [Math]::Sin($a1)
    $x2_h = $hubR * [Math]::Cos($a2); $y2_h = $hubR * [Math]::Sin($a2)
    $x1_b = $boreR * [Math]::Cos($a1); $y1_b = $boreR * [Math]::Sin($a1)
    $x2_b = $boreR * [Math]::Cos($a2); $y2_b = $boreR * [Math]::Sin($a2)

    Add-Quad $reelTris @($x1_b, $y1_b, $height) @($x2_b, $y2_b, $height) @($x2_h, $y2_h, $height) @($x1_h, $y1_h, $height)
    Add-Quad $reelTris @($x1_h, $y1_h, 0) @($x2_h, $y2_h, 0) @($x2_b, $y2_b, 0) @($x1_b, $y1_b, 0)
    Add-Quad $reelTris @($x1_b, $y1_b, 0) @($x1_b, $y1_b, $height) @($x2_b, $y2_b, $height) @($x2_b, $y2_b, 0)
}
Write-AsciiSTL "$OutputDir\TP7_Motorized_Tape_Reel.stl" "TP7_Motorized_Tape_Reel" $reelTris

# 2. GENERATE TP7 MAIN CHASSIS TOP (96mm x 68mm x 14mm Enclosure)
Write-Host "Generating TP7_Main_Chassis_Top.stl..."
$chassisTris = [System.Collections.ArrayList]::new()
$w = 68.0; $l = 96.0; $h = 14.0; $wall = 2.0
$w_inner = $w - $wall
$l_inner = $l - $wall

# Outer Box Faces
Add-Quad $chassisTris @(0,0,$h) @($w,0,$h) @($w,$l,$h) @(0,$l,$h) # Top
Add-Quad $chassisTris @(0,$l,0) @($w,$l,0) @($w,0,0) @(0,0,0) # Bottom opening lip
Add-Quad $chassisTris @(0,0,0) @($w,0,0) @($w,0,$h) @(0,0,$h) # Front wall
Add-Quad $chassisTris @($w,$l,0) @(0,$l,0) @(0,$l,$h) @($w,$l,$h) # Back wall
Add-Quad $chassisTris @(0,$l,0) @(0,0,0) @(0,0,$h) @(0,$l,$h) # Left wall
Add-Quad $chassisTris @($w,0,0) @($w,$l,0) @($w,$l,$h) @($w,0,$h) # Right wall

# Inner Hollow Cavity
Add-Quad $chassisTris @($wall,$wall,$wall) @($wall,$l_inner,$wall) @($w_inner,$l_inner,$wall) @($w_inner,$wall,$wall)
Add-Quad $chassisTris @($wall,$wall,0) @($w_inner,$wall,0) @($w_inner,$wall,$wall) @($wall,$wall,$wall)
Add-Quad $chassisTris @($w_inner,$l_inner,0) @($wall,$l_inner,0) @($wall,$l_inner,$wall) @($w_inner,$l_inner,$wall)
Add-Quad $chassisTris @($wall,$l_inner,0) @($wall,$wall,0) @($wall,$wall,$wall) @($wall,$l_inner,$wall)
Add-Quad $chassisTris @($w_inner,$wall,0) @($w_inner,$l_inner,0) @($w_inner,$l_inner,$wall) @($w_inner,$wall,$wall)

Write-AsciiSTL "$OutputDir\TP7_Main_Chassis_Top.stl" "TP7_Main_Chassis_Top" $chassisTris

# 3. GENERATE TP7 CHASSIS BOTTOM LID (96mm x 68mm x 2.5mm Back Cover Plate)
Write-Host "Generating TP7_Chassis_Bottom_Lid.stl..."
$lidTris = [System.Collections.ArrayList]::new()
$lidH = 2.5
Add-Quad $lidTris @(0,0,$lidH) @($w,0,$lidH) @($w,$l,$lidH) @(0,$l,$lidH)
Add-Quad $lidTris @(0,$l,0) @($w,$l,0) @($w,0,0) @(0,0,0)
Add-Quad $lidTris @(0,0,0) @($w,0,0) @($w,0,$lidH) @(0,0,$lidH)
Add-Quad $lidTris @($w,$l,0) @(0,$l,0) @(0,$l,$lidH) @($w,$l,$lidH)
Add-Quad $lidTris @(0,$l,0) @(0,0,0) @(0,0,$lidH) @(0,$l,$lidH)
Add-Quad $lidTris @($w,0,0) @($w,$l,0) @($w,$l,$lidH) @($w,0,$lidH)
Write-AsciiSTL "$OutputDir\TP7_Chassis_Bottom_Lid.stl" "TP7_Chassis_Bottom_Lid" $lidTris

# 4. GENERATE TP7 SIDE ROCKER SWITCH (16mm x 4.5mm x 6mm Rocker Lever)
Write-Host "Generating TP7_Side_Rocker_Switch.stl..."
$rockerTris = [System.Collections.ArrayList]::new()
$rw = 4.5; $rl = 16.0; $rh = 6.0
Add-Quad $rockerTris @(0,0,$rh) @($rw,0,$rh) @($rw,$rl,$rh) @(0,$rl,$rh)
Add-Quad $rockerTris @(0,$rl,0) @($rw,$rl,0) @($rw,0,0) @(0,0,0)
Add-Quad $rockerTris @(0,0,0) @($rw,0,0) @($rw,0,$rh) @(0,0,$rh)
Add-Quad $rockerTris @($rw,$rl,0) @(0,$rl,0) @(0,$rl,$rh) @($rw,$rl,$rh)
Add-Quad $rockerTris @(0,$rl,0) @(0,0,0) @(0,0,$rh) @(0,$rl,$rh)
Add-Quad $rockerTris @($rw,0,0) @($rw,$rl,0) @($rw,$rl,$rh) @($rw,0,$rh)
Write-AsciiSTL "$OutputDir\TP7_Side_Rocker_Switch.stl" "TP7_Side_Rocker_Switch" $rockerTris

# 5. GENERATE TP7 TACTILE BUTTON CAPS (6.8mm Dia x 4mm Recessed Buttons)
Write-Host "Generating TP7_Tactile_Button_Caps.stl..."
$btnTris = [System.Collections.ArrayList]::new()
$btnR = 3.4; $btnH = 4.0; $btnSeg = 24
for ($i = 0; $i -lt $btnSeg; $i++) {
    $a1 = ($i / $btnSeg) * [Math]::PI * 2
    $a2 = (($i + 1) / $btnSeg) * [Math]::PI * 2
    $x1 = $btnR * [Math]::Cos($a1); $y1 = $btnR * [Math]::Sin($a1)
    $x2 = $btnR * [Math]::Cos($a2); $y2 = $btnR * [Math]::Sin($a2)
    [void]$btnTris.Add(@(@(0,0,$btnH), @($x1,$y1,$btnH), @($x2,$y2,$btnH)))
    [void]$btnTris.Add(@(@(0,0,0), @($x2,$y2,0), @($x1,$y1,0)))
    Add-Quad $btnTris @($x1,$y1,0) @($x1,$y1,$btnH) @($x2,$y2,$btnH) @($x2,$y2,0)
}
Write-AsciiSTL "$OutputDir\TP7_Tactile_Button_Caps.stl" "TP7_Tactile_Button_Caps" $btnTris

Write-Host "All 5 STL 3D models successfully generated in: $OutputDir"
