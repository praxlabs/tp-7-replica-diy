import os
import sys
from reportlab.lib.pagesizes import letter
from reportlab.lib import colors
from reportlab.lib.units import inch
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak, KeepTogether, HRFlowable
)
from reportlab.pdfgen import canvas

class NumberedCanvas(canvas.Canvas):
    """
    Two-pass canvas to dynamically compute and draw total page numbers and running headers/footers.
    """
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._saved_page_states = []

    def showPage(self):
        self._saved_page_states.append(dict(self.__dict__))
        self._startPage()

    def save(self):
        num_pages = len(self._saved_page_states)
        for state in self._saved_page_states:
            self.__dict__.update(state)
            self.draw_header_footer(num_pages)
            super().showPage()
        super().save()

    def draw_header_footer(self, page_count):
        self.saveState()
        
        # Omit header on Cover Page (Page 1)
        if self._pageNumber > 1:
            self.setFont("Helvetica-Bold", 8)
            self.setFillColor(colors.HexColor("#718096"))
            self.drawString(54, 11 * inch - 36, "COMMERCIALIZATION, REGULATORY & LEGAL BLUEPRINT")
            self.setFont("Helvetica", 8)
            self.drawRightString(8.5 * inch - 54, 11 * inch - 36, "DIY TP-7 MOTORIZED RECORDER")
            self.setStrokeColor(colors.HexColor("#E2E8F0"))
            self.setLineWidth(0.75)
            self.line(54, 11 * inch - 42, 8.5 * inch - 54, 11 * inch - 42)

        # Running Footer on all pages
        self.setFont("Helvetica", 8)
        self.setFillColor(colors.HexColor("#718096"))
        self.drawString(54, 36, "CONFIDENTIAL & PROPRIETARY  |  COMMERCIAL HARDWARE COMPLIANCE GUIDE")
        page_str = f"Page {self._pageNumber} of {page_count}"
        self.drawRightString(8.5 * inch - 54, 36, page_str)
        self.setStrokeColor(colors.HexColor("#E2E8F0"))
        self.setLineWidth(0.75)
        self.line(54, 46, 8.5 * inch - 54, 46)
        
        self.restoreState()

def generate_pdf(output_filename):
    doc = SimpleDocTemplate(
        output_filename,
        pagesize=letter,
        leftMargin=54,
        rightMargin=54,
        topMargin=54,
        bottomMargin=54
    )

    styles = getSampleStyleSheet()

    # Custom Palette
    c_primary = colors.HexColor("#1A202C")   # Deep Slate Navy
    c_accent = colors.HexColor("#DD6B20")    # Industrial Orange
    c_secondary = colors.HexColor("#2B6CB0") # Cobalt Blue
    c_dark = colors.HexColor("#2D3748")      # Dark Grey Body
    c_bg_light = colors.HexColor("#F7FAFC")  # Light Grey Box
    c_border = colors.HexColor("#CBD5E0")    # Border

    # Typography Styles
    title_style = ParagraphStyle(
        'CoverTitle',
        fontName='Helvetica-Bold',
        fontSize=24,
        leading=28,
        textColor=c_primary,
        spaceAfter=8
    )
    subtitle_style = ParagraphStyle(
        'CoverSubtitle',
        fontName='Helvetica-Bold',
        fontSize=12,
        leading=16,
        textColor=c_accent,
        spaceAfter=14
    )
    meta_style = ParagraphStyle(
        'CoverMeta',
        fontName='Helvetica',
        fontSize=9,
        leading=13,
        textColor=colors.HexColor("#718096"),
        spaceAfter=20
    )
    h1_style = ParagraphStyle(
        'Heading1',
        fontName='Helvetica-Bold',
        fontSize=15,
        leading=19,
        textColor=c_primary,
        spaceBefore=14,
        spaceAfter=8,
        keepWithNext=True
    )
    h2_style = ParagraphStyle(
        'Heading2',
        fontName='Helvetica-Bold',
        fontSize=11,
        leading=15,
        textColor=c_secondary,
        spaceBefore=10,
        spaceAfter=5,
        keepWithNext=True
    )
    body_style = ParagraphStyle(
        'BodyDark',
        fontName='Helvetica',
        fontSize=9.5,
        leading=14,
        textColor=c_dark,
        spaceAfter=6
    )
    bullet_style = ParagraphStyle(
        'BulletText',
        fontName='Helvetica',
        fontSize=9.5,
        leading=13.5,
        textColor=c_dark,
        leftIndent=14,
        firstLineIndent=-10,
        spaceAfter=4
    )
    callout_style = ParagraphStyle(
        'CalloutText',
        fontName='Helvetica-Bold',
        fontSize=9,
        leading=13,
        textColor=c_primary
    )
    callout_body = ParagraphStyle(
        'CalloutBody',
        fontName='Helvetica',
        fontSize=8.5,
        leading=12.5,
        textColor=c_dark
    )
    table_hdr_style = ParagraphStyle(
        'TableHdr',
        fontName='Helvetica-Bold',
        fontSize=8.5,
        leading=11,
        textColor=colors.white
    )
    table_cell_style = ParagraphStyle(
        'TableCell',
        fontName='Helvetica',
        fontSize=8,
        leading=11,
        textColor=c_dark
    )
    table_cell_bold = ParagraphStyle(
        'TableCellBold',
        fontName='Helvetica-Bold',
        fontSize=8,
        leading=11,
        textColor=c_primary
    )

    story = []

    # =========================================================================
    # COVER / HEADER SECTION
    # =========================================================================
    story.append(Paragraph("HARDWARE COMMERCIALIZATION & LEGAL COMPLIANCE GUIDE", title_style))
    story.append(Paragraph("A Comprehensive Legal, Regulatory, Intellectual Property & Manufacturing Blueprint for Selling a Motorized-Reel Field Recorder", subtitle_style))
    story.append(Paragraph("<b>Author:</b> Product Architecture & Regulatory Advisory Team &nbsp;|&nbsp; <b>Project:</b> DIY TP-7 Digital Twin & Hardware Platform &nbsp;|&nbsp; <b>Classification:</b> Confidential Strategy", meta_style))
    story.append(HRFlowable(width="100%", thickness=2, color=c_accent, spaceAfter=14))

    # Executive Callout Box
    summary_html = """
    <b>EXECUTIVE SUMMARY & CRITICAL WARNING:</b><br/>
    Transitioning a DIY electronics project into a commercial consumer hardware product involves navigating strict intellectual property protections (trade dress, patents, trademarks), mandatory wireless certifications (FCC, CE, UKCA), lithium battery transport regulations (UN 38.3), and strict product liability insurance requirements. Building a functional prototype is ~15% of the effort; 85% of commercialization cost resides in DFM, regulatory compliance, supply chain scaling, and legal de-risking.
    """
    callout_data = [[Paragraph(summary_html, callout_body)]]
    callout_table = Table(callout_data, colWidths=[504])
    callout_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), colors.HexColor("#FEFCBF")), # Light Amber
        ('BOX', (0,0), (-1,-1), 1, colors.HexColor("#D69E2E")),
        ('TOPPADDING', (0,0), (-1,-1), 8),
        ('BOTTOMPADDING', (0,0), (-1,-1), 8),
        ('LEFTPADDING', (0,0), (-1,-1), 10),
        ('RIGHTPADDING', (0,0), (-1,-1), 10),
    ]))
    story.append(callout_table)
    story.append(Spacer(1, 14))

    # =========================================================================
    # 1. INTELLECTUAL PROPERTY (IP) & LEGAL RISKS
    # =========================================================================
    story.append(Paragraph("1. Intellectual Property (IP) & Legal Pitfalls (Teenage Engineering IP)", h1_style))
    story.append(Paragraph(
        "When commercializing hardware inspired by an existing market leader (such as Teenage Engineering's TP-7), you must navigate four distinct categories of Intellectual Property law. Infringement in any of these areas can result in immediate Cease-and-Desist letters, customs asset seizures, and statutory damages.",
        body_style
    ))

    story.append(Paragraph("A. Trademark Infringement & Brand Protection (Absolute Prohibition)", h2_style))
    story.append(Paragraph(
        "• <b>Forbidden Terms:</b> You cannot use 'TP-7', 'Teenage Engineering', 'Field System', 'TX-6', 'OP-1', or similar trademarks in product names, domain names, packaging, promotional media, or metadata. Doing so constitutes direct trademark infringement under the Lanham Act (15 U.S.C. § 1114).<br/>"
        "• <b>Permitted Comparative Statements:</b> You may only state 'Compatible with TRRS audio accessories' or 'Alternative motorized field recorder'. Do not describe the product as a 'TP-7 Clone' or 'DIY TP-7 Commercial Replica'.<br/>"
        "• <b>Independent Branding:</b> You must create an original brand name (e.g., <i>'SpoolCorder Pro'</i>, <i>'PulseReel 7'</i>) and register standard word and design marks with the USPTO / EUIPO.",
        bullet_style
    ))

    story.append(Paragraph("B. Trade Dress & Design Patents (Aesthetic Form Factor)", h2_style))
    story.append(Paragraph(
        "Trade dress protects the overall visual appearance and consumer recognition of a product. Teenage Engineering is renowned for aggressively defending its iconic design language (anodized aluminum unibody, specific circular spool proportion, distinct orange recessed buttons, minimalist chamfered geometry).",
        body_style
    ))
    story.append(Paragraph(
        "• <b>Design Differentiation Checklist:</b><br/>"
        "&nbsp;&nbsp;1. Modify the exterior chassis dimensions and corner radiuses (avoid exact 1:1 aspect ratio replication).<br/>"
        "&nbsp;&nbsp;2. Use a distinct button layout and alternative accent colorways (e.g., cobalt blue or anodized brass instead of iconic TE orange).<br/>"
        "&nbsp;&nbsp;3. Re-engineer the tape reel hub design (e.g., 4-spoke or 5-spoke geometry rather than the signature 3-spoke TE spool cutouts).",
        bullet_style
    ))

    story.append(Paragraph("C. Utility Patents & Mechanical Claims (Touch-to-Pause & Scrub Mechanisms)", h2_style))
    story.append(Paragraph(
        "Teenage Engineering holds patents on motorized haptic jog dials, magnetic slip clutches, and tactile shuttle rockers. To avoid utility patent infringement:<br/>"
        "• <b>Prior Art Search:</b> Perform a thorough FTO (Freedom-to-Operate) patent search via Google Patents and Espacenet covering classification codes <i>G11B (Recording/Playback)</i> and <i>H04R (Audio Transducers)</i>.<br/>"
        "• <b>Algorithmic Distinction:</b> Use software-based Hall sensor angular differentiation (detecting velocity drop on the AS5600) rather than proprietary mechanical brake clutch hardware.",
        body_style
    ))

    story.append(Spacer(1, 10))

    # =========================================================================
    # 2. REGULATORY COMPLIANCE & WIRELESS CERTIFICATIONS
    # =========================================================================
    story.append(Paragraph("2. Mandatory Regulatory Compliance & Certifications", h1_style))
    story.append(Paragraph(
        "Selling electronic hardware legally requires passing standardized electromagnetic compatibility (EMC) and radio frequency (RF) testing. Shipping uncertified hardware carries civil fines of up to $25,000 per violation day.",
        body_style
    ))

    cert_table_data = [
        [Paragraph("Certification / Region", table_hdr_style), Paragraph("Requirement & Scope", table_hdr_style), Paragraph("Testing Cost (Est.)", table_hdr_style), Paragraph("Modular Strategy", table_hdr_style)],
        [
            Paragraph("<b>FCC (USA)</b><br/>Part 15 Class B", table_cell_bold),
            Paragraph("Electromagnetic emissions & intentional RF broadcast (Bluetooth / BLE).", table_cell_style),
            Paragraph("$2,500 – $4,500 (Modular)<br/>$12,000 – $18,000 (Full)", table_cell_style),
            Paragraph("Use pre-certified <b>ESP32-WROOM-32E</b> (FCC ID: 2AC7Z-ESPWROOM32E) to bypass full intentional radiator RF testing.", table_cell_style)
        ],
        [
            Paragraph("<b>CE Marking (EU)</b><br/>RED, EMC, RoHS", table_cell_bold),
            Paragraph("Radio Equipment Directive (2014/53/EU), EMC Directive, and RoHS 3 hazardous substance restrictions.", table_cell_style),
            Paragraph("€4,000 – €7,500", table_cell_style),
            Paragraph("Requires Declaration of Conformity (DoC), Technical Construction File (TCF), and lead-free ENIG PCB fabrication.", table_cell_style)
        ],
        [
            Paragraph("<b>UKCA (UK)</b>", table_cell_bold),
            Paragraph("UK Radio Equipment Regulations post-Brexit.", table_cell_style),
            Paragraph("£1,500 – £3,000", table_cell_style),
            Paragraph("Direct alignment with CE test reports with UKCA technical file.", table_cell_style)
        ],
        [
            Paragraph("<b>ISED / IC (Canada)</b>", table_cell_bold),
            Paragraph("Innovation, Science and Economic Development Canada (RSS-247).", table_cell_style),
            Paragraph("$1,800 – $3,000", table_cell_style),
            Paragraph("ESP32 module carries Canadian IC pre-certification.", table_cell_style)
        ]
    ]

    cert_table = Table(cert_table_data, colWidths=[110, 140, 100, 154])
    cert_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), c_primary),
        ('GRID', (0,0), (-1,-1), 0.5, c_border),
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('TOPPADDING', (0,0), (-1,-1), 5),
        ('BOTTOMPADDING', (0,0), (-1,-1), 5),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, c_bg_light]),
    ]))
    story.append(cert_table)
    story.append(Spacer(1, 12))

    # =========================================================================
    # 3. LITHIUM BATTERY SAFETY & TRANSPORT LAWS
    # =========================================================================
    story.append(Paragraph("3. Lithium Battery Safety, Transport & Environmental Laws", h1_style))
    story.append(Paragraph(
        "Lithium-Polymer (LiPo) batteries are classified as Dangerous Goods (Class 9). Commercial sale and shipping requires strict compliance to prevent thermal runaway fires.",
        body_style
    ))
    story.append(Paragraph(
        "• <b>UN 38.3 Transport Certification:</b> Required by all air (IATA/ICAO) and ground carriers (USPS, FedEx, DHL) for shipping devices containing integrated lithium batteries. Testing subjects cells to altitude simulation, thermal shock, vibration, impact, and external short-circuiting.<br/>"
        "• <b>Dual Protection Hardware:</b> Commercial boards must feature an integrated battery protection IC (e.g., DW01A or Texas Instruments BQ24075) with over-voltage, under-voltage, over-current, and <b>NTC thermistor temperature sensing</b> to halt charging if battery exceeds 45°C.<br/>"
        "• <b>WEEE & California Proposition 65:</b> Must include the crossed-out wheelie bin symbol on packaging and mandatory chemical exposure warning labels if lead or specific phthalates are present.",
        bullet_style
    ))

    story.append(Spacer(1, 10))

    # =========================================================================
    # 4. SOFTWARE, FIRMWARE & AUDIO CODEC LICENSING
    # =========================================================================
    story.append(Paragraph("4. Software, Firmware & Audio Codec Licensing", h1_style))
    story.append(Paragraph(
        "• <b>Open-Source License Segregation:</b> The ESP32 ESP-IDF framework is distributed under Apache 2.0 (permissive for commercialization). However, ensure no GPL v3.0 libraries are statically linked into your production firmware, which could legally obligate you to release your proprietary firmware source code to customers.<br/>"
        "• <b>Audio Codec Patent Pools:</b> Uncompressed 16/24-bit <b>Linear PCM (.WAV)</b> and <b>FLAC</b> are royalty-free. If you implement compressed recording (MP3 or AAC), you must verify licensing terms with patent pools (Via Licensing Alliance). Recording uncompressed WAV is standard for pro field recorders and completely avoids patent royalty overhead.",
        bullet_style
    ))

    story.append(Spacer(1, 10))

    # =========================================================================
    # 5. MANUFACTURING ECONOMICS, DFM & PRICING FORMULA
    # =========================================================================
    story.append(Paragraph("5. Manufacturing Economics, DFM & Target Unit Pricing", h1_style))
    story.append(Paragraph(
        "A healthy hardware business relies on the <b>Hardware Multiplier Rule</b>: Retail Price (MSRP) must be <b>3.5x to 4.5x</b> the Bill of Materials (BOM) cost to cover manufacturing scrap, tooling amortization, logistics, warranty reserves, and retail channel margins.",
        body_style
    ))

    bom_cost_data = [
        [Paragraph("Cost Component", table_hdr_style), Paragraph("Batch 50 Units (DIY)", table_hdr_style), Paragraph("Batch 1,000 Units (Pilot)", table_hdr_style), Paragraph("Batch 10,000 Units (Scale)", table_hdr_style)],
        [Paragraph("Electronics BOM (ESP32, Mic, Codec, Motor, Sensors)", table_cell_bold), Paragraph("$38.50", table_cell_style), Paragraph("$22.40", table_cell_style), Paragraph("$14.80", table_cell_style)],
        [Paragraph("PCB Fabrication + SMT Assembly (JLCPCB/PCBWay)", table_cell_bold), Paragraph("$18.00", table_cell_style), Paragraph("$7.50", table_cell_style), Paragraph("$3.80", table_cell_style)],
        [Paragraph("Enclosure & Mechanism (CNC Aluminum / Tooling)", table_cell_bold), Paragraph("$24.00 (MJF 3D)", table_cell_style), Paragraph("$16.50 (CNC)", table_cell_style), Paragraph("$6.20 (Die-Cast / Anodized)", table_cell_style)],
        [Paragraph("Battery (1000mAh UN38.3 Certified)", table_cell_bold), Paragraph("$6.50", table_cell_style), Paragraph("$3.80", table_cell_style), Paragraph("$2.40", table_cell_style)],
        [Paragraph("Packaging, USB-C Cable & Manuals", table_cell_bold), Paragraph("$4.00", table_cell_style), Paragraph("$2.80", table_cell_style), Paragraph("$1.60", table_cell_style)],
        [Paragraph("<b>Total Unit COGS (Cost of Goods Sold)</b>", table_cell_bold), Paragraph("<b>$91.00</b>", table_cell_bold), Paragraph("<b>$53.00</b>", table_cell_bold), Paragraph("<b>$28.80</b>", table_cell_bold)],
        [Paragraph("<b>Target Retail Price (MSRP)</b>", table_cell_bold), Paragraph("<b>$249.00 (2.7x)</b>", table_cell_bold), Paragraph("<b>$199.00 (3.8x)</b>", table_cell_bold), Paragraph("<b>$129.00 (4.5x)</b>", table_cell_bold)]
    ]

    bom_table = Table(bom_cost_data, colWidths=[180, 108, 108, 108])
    bom_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), c_primary),
        ('GRID', (0,0), (-1,-1), 0.5, c_border),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
        ('TOPPADDING', (0,0), (-1,-1), 4),
        ('BOTTOMPADDING', (0,0), (-1,-1), 4),
        ('ROWBACKGROUNDS', (0,1), (-1,-2), [colors.white, c_bg_light]),
        ('BACKGROUND', (0,-2), (-1,-1), colors.HexColor("#E2E8F0")),
    ]))
    story.append(bom_table)
    story.append(Spacer(1, 12))

    # =========================================================================
    # 6. PRODUCT LIABILITY, WARRANTIES & INSURANCE
    # =========================================================================
    story.append(Paragraph("6. Product Liability, Warranties & Insurance", h1_style))
    story.append(Paragraph(
        "• <b>Commercial General & Product Liability Insurance:</b> Obtain at least $1,000,000 to $2,000,000 in product liability coverage before shipping any consumer electronics with lithium-ion batteries.<br/>"
        "• <b>Limited 1-Year Consumer Warranty:</b> Draft clear warranty terms excluding unauthorized teardowns, water damage, or excessive motor torque abuse.<br/>"
        "• <b>RMA (Return Merchandise Authorization) Reserve:</b> Budget 3% to 5% of gross revenue for warranty replacements, field returns, and component failure triage.",
        bullet_style
    ))

    story.append(Spacer(1, 10))

    # =========================================================================
    # 7. ACTIONABLE PRE-LAUNCH CHECKLIST
    # =========================================================================
    story.append(Paragraph("7. Actionable Commercialization Checklist", h1_style))
    
    checklist_data = [
        [Paragraph("Phase", table_hdr_style), Paragraph("Action Item", table_hdr_style), Paragraph("Status", table_hdr_style)],
        [Paragraph("1. Legal", table_cell_bold), Paragraph("Trademark original brand name (USPTO/EUIPO) & verify trade dress clearance.", table_cell_style), Paragraph("[  ] Required", table_cell_style)],
        [Paragraph("2. Engineering", table_cell_bold), Paragraph("Freeze 4-layer custom PCB design with pre-certified ESP32-WROOM-32E module.", table_cell_style), Paragraph("[  ] Required", table_cell_style)],
        [Paragraph("3. Battery Safety", table_cell_bold), Paragraph("Source UN 38.3 and IEC 62133 certified LiPo cells with onboard NTC protection.", table_cell_style), Paragraph("[  ] Required", table_cell_style)],
        [Paragraph("4. Regulatory", table_cell_bold), Paragraph("Submit 3 pilot units to accredited test lab for FCC Part 15 / CE RED emissions testing.", table_cell_style), Paragraph("[  ] Required", table_cell_style)],
        [Paragraph("5. Insurance", table_cell_bold), Paragraph("Bind commercial product liability policy with hardware e-commerce rider.", table_cell_style), Paragraph("[  ] Required", table_cell_style)],
        [Paragraph("6. Packaging", table_cell_bold), Paragraph("Print FCC ID, CE logo, WEEE bin symbol, and ratings on rear chassis laser etching.", table_cell_style), Paragraph("[  ] Required", table_cell_style)]
    ]
    check_table = Table(checklist_data, colWidths=[80, 344, 80])
    check_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), c_primary),
        ('GRID', (0,0), (-1,-1), 0.5, c_border),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
        ('TOPPADDING', (0,0), (-1,-1), 4),
        ('BOTTOMPADDING', (0,0), (-1,-1), 4),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, c_bg_light]),
    ]))
    story.append(check_table)

    # Build Document with NumberedCanvas
    doc.build(story, canvasmaker=NumberedCanvas)
    print(f"Successfully generated PDF: {output_filename}")

if __name__ == "__main__":
    out_path = os.path.abspath("DIY-TP7-Commercialization-And-Legal-Guide.pdf")
    generate_pdf(out_path)
