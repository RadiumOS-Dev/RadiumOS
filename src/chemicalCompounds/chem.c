#include "../keyboard/keyboard.h"
#include "../terminal/terminal.h"
#include "../utility/utility.h"
#include "../timers/timer.h"
#include "../io/io.h"

// Chemistry compound structure
typedef struct {
    char name[32];
    char formula[16];
    char description[128];
    char properties[256];
} compound_t;

void display_menu(compound_t compounds[], int current_selection, int max_compounds) {
    terminal_clear();
    print("\n=== COMPLETE PERIODIC TABLE DATABASE ===\n\n");
    
    // Show 10 elements at a time for better visibility
    int start = (current_selection / 10) * 10;
    int end = start + 10;
    if (end > max_compounds) end = max_compounds;
    
    for (int i = start; i < end; i++) {
        if (i == current_selection) {
            print(">>> ");  // Highlight current selection
        } else {
            print("    ");
        }
        print(compounds[i].name);
        print(" (");
        print(compounds[i].formula);
        print(")\n");
    }
    
    print("\nShowing ");
    char start_str[8], end_str[8], total_str[8];
    itoa(start + 1, start_str, 10);
    itoa(end, end_str, 10);
    itoa(max_compounds, total_str, 10);
    print(start_str);
    print("-");
    print(end_str);
    print(" of ");
    print(total_str);
    print(" elements\n");
    
    print("\nControls: A=Up, D=Down, SPACE=Select, ESC=Exit\n");
    print("Current: ");
    print(compounds[current_selection].name);
    print("\n");
}

void chem_command(int argc, char* argv[]) { 
    print("\nPeriodic Table Mode - Use A/D to navigate, SPACE to select, ESC to exit\n");
    
    // Complete Periodic Table - All 118 Elements
    compound_t compounds[118] = {
        // Period 1
        {"Hydrogen", "H", "Lightest element, fuel of stars", "Atomic: 1, Gas, Highly flammable, Colorless. SAFETY: Explosion risk"},
        {"Helium", "He", "Noble gas, second lightest element", "Atomic: 2, Gas, Inert, Used in balloons. SAFETY: Asphyxiant"},
        
        // Period 2
        {"Lithium", "Li", "Lightest metal, used in batteries", "Atomic: 3, Soft metal, Reactive. SAFETY: Flammable, corrosive"},
        {"Beryllium", "Be", "Light, strong metal", "Atomic: 4, Hard metal, Toxic dust. SAFETY: CARCINOGENIC"},
        {"Boron", "B", "Metalloid, used in glass", "Atomic: 5, Semiconductor, Hard. SAFETY: Mildly toxic"},
        {"Carbon", "C", "Basis of organic chemistry", "Atomic: 6, Forms diamonds/graphite. SAFETY: Generally safe"},
        {"Nitrogen", "N", "Makes up 78% of atmosphere", "Atomic: 7, Gas, Inert as N2. SAFETY: Asphyxiant"},
        {"Oxygen", "O", "Essential for life", "Atomic: 8, Gas, Supports combustion. SAFETY: Fire hazard"},
        {"Fluorine", "F", "Most reactive element", "Atomic: 9, Gas, Extremely reactive. SAFETY: EXTREMELY TOXIC"},
        {"Neon", "Ne", "Noble gas, used in signs", "Atomic: 10, Gas, Inert, Orange glow. SAFETY: Asphyxiant"},
        
        // Period 3
        {"Sodium", "Na", "Alkali metal, explosive in water", "Atomic: 11, Soft metal, Highly reactive. SAFETY: Explosive with water"},
        {"Magnesium", "Mg", "Light metal, burns bright white", "Atomic: 12, Metal, Burns brilliantly. SAFETY: Fire hazard"},
        {"Aluminum", "Al", "Most abundant metal in crust", "Atomic: 13, Light metal, Corrosion resistant. SAFETY: Generally safe"},
        {"Silicon", "Si", "Semiconductor, computer chips", "Atomic: 14, Metalloid, Forms silicates. SAFETY: Dust harmful"},
        {"Phosphorus", "P", "Essential for life, matches", "Atomic: 15, Nonmetal, Reactive. SAFETY: Toxic, flammable"},
        {"Sulfur", "S", "Yellow solid, volcanic", "Atomic: 16, Nonmetal, Forms compounds. SAFETY: Irritant"},
        {"Chlorine", "Cl", "Halogen, water disinfectant", "Atomic: 17, Gas, Greenish, Toxic. SAFETY: TOXIC GAS"},
        {"Argon", "Ar", "Noble gas, inert atmosphere", "Atomic: 18, Gas, Inert, Most abundant noble gas. SAFETY: Asphyxiant"},
        
        // Period 4
        {"Potassium", "K", "Alkali metal, essential nutrient", "Atomic: 19, Soft metal, Explosive in water. SAFETY: Explosive"},
        {"Calcium", "Ca", "Alkaline earth, bones/teeth", "Atomic: 20, Metal, Essential for life. SAFETY: Generally safe"},
        {"Scandium", "Sc", "Rare earth metal", "Atomic: 21, Metal, Light, Expensive. SAFETY: Low toxicity"},
        {"Titanium", "Ti", "Strong, light, corrosion resistant", "Atomic: 22, Metal, Aerospace use. SAFETY: Generally safe"},
        {"Vanadium", "V", "Steel additive", "Atomic: 23, Metal, Hardens steel. SAFETY: Toxic compounds"},
        {"Chromium", "Cr", "Stainless steel component", "Atomic: 24, Metal, Corrosion resistant. SAFETY: Cr(VI) carcinogenic"},
        {"Manganese", "Mn", "Steel production", "Atomic: 25, Metal, Essential trace element. SAFETY: Neurotoxic"},
        {"Iron", "Fe", "Most used metal", "Atomic: 26, Metal, Magnetic, Essential. SAFETY: Generally safe"},
        {"Cobalt", "Co", "Magnetic metal, blue pigment", "Atomic: 27, Metal, Magnetic. SAFETY: Carcinogenic"},
        {"Nickel", "Ni", "Corrosion resistant, coins", "Atomic: 28, Metal, Magnetic. SAFETY: Carcinogenic"},
        {"Copper", "Cu", "Electrical conductor", "Atomic: 29, Metal, Reddish, Antimicrobial. SAFETY: Generally safe"},
        {"Zinc", "Zn", "Galvanizing, essential nutrient", "Atomic: 30, Metal, Bluish-white. SAFETY: Generally safe"},
        {"Gallium", "Ga", "Melts in hand, semiconductors", "Atomic: 31, Metal, Low melting point. SAFETY: Low toxicity"},
        {"Germanium", "Ge", "Semiconductor", "Atomic: 32, Metalloid, Electronics. SAFETY: Low toxicity"},
        {"Arsenic", "As", "Metalloid, poison", "Atomic: 33, Metalloid, Toxic. SAFETY: HIGHLY TOXIC"},
        {"Selenium", "Se", "Essential trace element", "Atomic: 34, Nonmetal, Photoconductor. SAFETY: Toxic in excess"},
        {"Bromine", "Br", "Liquid halogen", "Atomic: 35, Liquid, Red-brown, Toxic. SAFETY: CORROSIVE"},
        {"Krypton", "Kr", "Noble gas", "Atomic: 36, Gas, Inert. SAFETY: Asphyxiant"},
        
        // Period 5
        {"Rubidium", "Rb", "Alkali metal", "Atomic: 37, Metal, Highly reactive. SAFETY: Explosive with water"},
        {"Strontium", "Sr", "Alkaline earth, fireworks", "Atomic: 38, Metal, Red flame color. SAFETY: Radioactive isotopes"},
        {"Yttrium", "Y", "Rare earth", "Atomic: 39, Metal, Superconductors. SAFETY: Low toxicity"},
        {"Zirconium", "Zr", "Nuclear reactors", "Atomic: 40, Metal, Corrosion resistant. SAFETY: Generally safe"},
        {"Niobium", "Nb", "Superconducting magnets", "Atomic: 41, Metal, Superconductor. SAFETY: Generally safe"},
        {"Molybdenum", "Mo", "Steel alloys", "Atomic: 42, Metal, High melting point. SAFETY: Generally safe"},
        {"Technetium", "Tc", "First artificial element", "Atomic: 43, Metal, Radioactive. SAFETY: RADIOACTIVE"},
        {"Ruthenium", "Ru", "Platinum group metal", "Atomic: 44, Metal, Catalyst. SAFETY: Toxic compounds"},
        {"Rhodium", "Ru", "Precious metal, catalysts", "Atomic: 45, Metal, Very expensive. SAFETY: Generally safe"},
        {"Palladium", "Pd", "Catalysts, jewelry", "Atomic: 46, Metal, Hydrogen storage. SAFETY: Generally safe"},
        {"Silver", "Ag", "Precious metal, antimicrobial", "Atomic: 47, Metal, Best conductor. SAFETY: Generally safe"},
        {"Cadmium", "Cd", "Batteries, pigments", "Atomic: 48, Metal, Toxic. SAFETY: HIGHLY TOXIC"},
        {"Indium", "In", "Touch screens", "Atomic: 49, Metal, Soft. SAFETY: Low toxicity"},
        {"Tin", "Sn", "Cans, solder", "Atomic: 50, Metal, Corrosion resistant. SAFETY: Generally safe"},
        {"Antimony", "Sb", "Flame retardants", "Atomic: 51, Metalloid, Toxic. SAFETY: TOXIC"},
        {"Tellurium", "Te", "Semiconductors", "Atomic: 52, Metalloid, Rare. SAFETY: Toxic"},
        {"Iodine", "I", "Antiseptic, thyroid function", "Atomic: 53, Halogen, Purple vapor. SAFETY: Toxic in excess"},
        {"Xenon", "Xe", "Noble gas, anesthesia", "Atomic: 54, Gas, Inert. SAFETY: Asphyxiant"},
        
        // Period 6
        {"Cesium", "Cs", "Most reactive metal", "Atomic: 55, Metal, Explosive in water. SAFETY: EXTREMELY REACTIVE"},
        {"Barium", "Ba", "X-ray contrast agent", "Atomic: 56, Metal, Dense. SAFETY: TOXIC"},
        {"Lanthanum", "La", "Rare earth, camera lenses", "Atomic: 57, Metal, Lanthanide. SAFETY: Low toxicity"},
        {"Cerium", "Ce", "Lighter flints", "Atomic: 58, Metal, Most abundant rare earth. SAFETY: Flammable"},
        {"Praseodymium", "Pr", "Magnets, lasers", "Atomic: 59, Metal, Green salts. SAFETY: Low toxicity"},
        {"Neodymium", "Nd", "Powerful magnets", "Atomic: 60, Metal, Purple salts. SAFETY: Eye hazard"},
        {"Promethium", "Pm", "Radioactive rare earth", "Atomic: 61, Metal, No stable isotopes. SAFETY: RADIOACTIVE"},
        {"Samarium", "Sm", "Magnets", "Atomic: 62, Metal, Yellow salts. SAFETY: Low toxicity"},
        {"Europium", "Eu", "Red phosphor in TVs", "Atomic: 63, Metal, Most reactive rare earth. SAFETY: Low toxicity"},
        {"Gadolinium", "Gd", "MRI contrast agent", "Atomic: 64, Metal, Magnetic. SAFETY: Toxic compounds"},
        {"Terbium", "Tb", "Green phosphors", "Atomic: 65, Metal, Fluorescent. SAFETY: Low toxicity"},
        {"Dysprosium", "Dy", "Magnets, lasers", "Atomic: 66, Metal, High magnetic strength. SAFETY: Low toxicity"},
        {"Holmium", "Ho", "Strongest magnetic field", "Atomic: 67, Metal, Yellow salts. SAFETY: Low toxicity"},
        {"Erbium", "Er", "Fiber optic amplifiers", "Atomic: 68, Metal, Pink salts. SAFETY: Low toxicity"},
        {"Thulium", "Tm", "Portable X-ray sources", "Atomic: 69, Metal, Least abundant rare earth. SAFETY: RADIOACTIVE"},
        {"Ytterbium", "Yb", "Atomic clocks", "Atomic: 70, Metal, Soft. SAFETY: Low toxicity"},
        {"Lutetium", "Lu", "PET scan detectors", "Atomic: 71, Metal, Hardest rare earth. SAFETY: Low toxicity"},
        {"Hafnium", "Hf", "Nuclear reactor control", "Atomic: 72, Metal, Neutron absorber. SAFETY: Generally safe"},
        {"Tantalum", "Ta", "Capacitors, implants", "Atomic: 73, Metal, Corrosion resistant. SAFETY: Generally safe"},
        {"Tungsten", "W", "Light bulb filaments", "Atomic: 74, Metal, Highest melting point. SAFETY: Generally safe"},
        {"Rhenium", "Re", "Jet engine parts", "Atomic: 75, Metal, Very rare. SAFETY: Generally safe"},
        {"Osmium", "Os", "Densest element", "Atomic: 76, Metal, Extremely dense. SAFETY: Toxic oxide"},
        {"Iridium", "Ir", "Spark plugs, crucibles", "Atomic: 77, Metal, Very hard. SAFETY: Generally safe"},
        {"Platinum", "Pt", "Jewelry, catalysts", "Atomic: 78, Metal, Precious. SAFETY: Generally safe"},
        {"Gold", "Au", "Jewelry, electronics", "Atomic: 79, Metal, Noble, Unreactive. SAFETY: Generally safe"},
        {"Mercury", "Hg", "Liquid metal, thermometers", "Atomic: 80, Liquid metal, Toxic vapor. SAFETY: HIGHLY TOXIC"},
        {"Thallium", "Tl", "Electronics", "Atomic: 81, Metal, Very toxic. SAFETY: EXTREMELY TOXIC"},
        {"Lead", "Pb", "Batteries, radiation shielding", "Atomic: 82, Metal, Dense, Toxic. SAFETY: TOXIC"},
        {"Bismuth", "Bi", "Medicines, cosmetics", "Atomic: 83, Metal, Low toxicity. SAFETY: Generally safe"},
        {"Polonium", "Po", "Radioactive, discovered by Curie", "Atomic: 84, Metal, Highly radioactive. SAFETY: EXTREMELY RADIOACTIVE"},
        {"Astatine", "At", "Rarest natural element", "Atomic: 85, Halogen, Radioactive. SAFETY: RADIOACTIVE"},
        {"Radon", "Rn", "Radioactive gas from uranium", "Atomic: 86, Gas, Radioactive. SAFETY: CARCINOGENIC"},
        
        // Period 7
        {"Francium", "Fr", "Most unstable natural element", "Atomic: 87, Metal, Extremely radioactive. SAFETY: RADIOACTIVE"},
        {"Radium", "Ra", "Radioactive, glows in dark", "Atomic: 88, Metal, Radioactive. SAFETY: EXTREMELY RADIOACTIVE"},
                {"Actinium", "Ac", "Radioactive, cancer treatment", "Atomic: 89, Metal, Radioactive. SAFETY: RADIOACTIVE"},
        {"Thorium", "Th", "Nuclear fuel, gas mantles", "Atomic: 90, Metal, Radioactive. SAFETY: RADIOACTIVE"},
        {"Protactinium", "Pa", "Uranium decay product", "Atomic: 91, Metal, Highly radioactive. SAFETY: EXTREMELY RADIOACTIVE"},
        {"Uranium", "U", "Nuclear fuel, weapons", "Atomic: 92, Metal, Fissile. SAFETY: RADIOACTIVE, TOXIC"},
        {"Neptunium", "Np", "First transuranium element", "Atomic: 93, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Plutonium", "Pu", "Nuclear weapons, power", "Atomic: 94, Metal, Extremely toxic. SAFETY: EXTREMELY RADIOACTIVE"},
        {"Americium", "Am", "Smoke detectors", "Atomic: 95, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Curium", "Cm", "Research, space missions", "Atomic: 96, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Berkelium", "Bk", "Research only", "Atomic: 97, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Californium", "Cf", "Neutron source", "Atomic: 98, Metal, Artificial. SAFETY: EXTREMELY RADIOACTIVE"},
        {"Einsteinium", "Es", "Research only", "Atomic: 99, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Fermium", "Fm", "Research only", "Atomic: 100, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Mendelevium", "Md", "Research only", "Atomic: 101, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Nobelium", "No", "Research only", "Atomic: 102, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Lawrencium", "Lr", "Research only", "Atomic: 103, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Rutherfordium", "Rf", "Synthetic superheavy", "Atomic: 104, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Dubnium", "Db", "Synthetic superheavy", "Atomic: 105, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Seaborgium", "Sg", "Synthetic superheavy", "Atomic: 106, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Bohrium", "Bh", "Synthetic superheavy", "Atomic: 107, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Hassium", "Hs", "Synthetic superheavy", "Atomic: 108, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Meitnerium", "Mt", "Synthetic superheavy", "Atomic: 109, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Darmstadtium", "Ds", "Synthetic superheavy", "Atomic: 110, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Roentgenium", "Rg", "Synthetic superheavy", "Atomic: 111, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Copernicium", "Cn", "Synthetic superheavy", "Atomic: 112, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Nihonium", "Nh", "Synthetic superheavy", "Atomic: 113, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Flerovium", "Fl", "Synthetic superheavy", "Atomic: 114, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Moscovium", "Mc", "Synthetic superheavy", "Atomic: 115, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Livermorium", "Lv", "Synthetic superheavy", "Atomic: 116, Metal, Artificial. SAFETY: RADIOACTIVE"},
        {"Tennessine", "Ts", "Synthetic superheavy", "Atomic: 117, Halogen, Artificial. SAFETY: RADIOACTIVE"},
        {"Oganesson", "Og", "Heaviest known element", "Atomic: 118, Noble gas, Artificial. SAFETY: RADIOACTIVE"}
    };
    
    int current_selection = 0;
    int max_compounds = 118;
    bool need_redraw = true;
    
    // Display initial menu
    display_menu(compounds, current_selection, max_compounds);
    
    while (1) {
        // Only redraw if needed
        if (need_redraw) {
            display_menu(compounds, current_selection, max_compounds);
            need_redraw = false;
        }
        
        // Handle input
        if (is_key_pressed()) {
            uint8_t scan_code = port_byte_in(0x60);
            
            // Only process key press events (not releases)
            if (!(scan_code & 0x80)) {  // Check if it's a key press (not release)
                
                if (scan_code == 30) { // A key - Move up
                    current_selection--;
                    if (current_selection < 0) {
                        current_selection = max_compounds - 1; // Wrap to bottom
                    }
                    need_redraw = true;
                }
                else if (scan_code == 32) { // D key - Move down
                    current_selection++;
                    if (current_selection >= max_compounds) {
                        current_selection = 0; // Wrap to top
                    }
                    need_redraw = true;
                }
                else if (scan_code == 17) { // W key - Jump up 10
                    current_selection -= 10;
                    if (current_selection < 0) {
                        current_selection = 0;
                    }
                    need_redraw = true;
                }
                else if (scan_code == 31) { // S key - Jump down 10
                    current_selection += 10;
                    if (current_selection >= max_compounds) {
                        current_selection = max_compounds - 1;
                    }
                    need_redraw = true;
                }
                else if (scan_code == 57) { // SPACE key - Select
                    terminal_clear();
                    print("\n=== ELEMENT DETAILS ===\n");
                    print("Name: ");
                    print(compounds[current_selection].name);
                    print("\nSymbol: ");
                    print(compounds[current_selection].formula);
                    print("\nDescription: ");
                    print(compounds[current_selection].description);
                    print("\nProperties & Safety: ");
                    print(compounds[current_selection].properties);
                    print("\n\nPress any key to return to periodic table...\n");
                    
                    // Wait for any key press to return
                    while (1) {
                        if (is_key_pressed()) {
                            uint8_t return_key = port_byte_in(0x60);
                            if (!(return_key & 0x80)) { // Key press, not release
                                break;
                            }
                        }
                        // Small delay
                        for (volatile int i = 0; i < 10000; i++);
                    }
                    
                    need_redraw = true;
                }
                else if (scan_code == 1) { // ESC key - Exit
                    terminal_clear();
                    //print("\nExiting periodic table...\n");
                    return;
                }
                
                // Wait a bit to prevent key repeat
                for (volatile int i = 0; i < 100000; i++);
            }
        }
        
        // Small delay to prevent excessive CPU usage
        for (volatile int i = 0; i < 10000; i++);
    }
}
