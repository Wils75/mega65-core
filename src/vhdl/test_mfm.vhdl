library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use STD.textio.all;
use work.debugtools.all;

entity test_mfm is
end entity;

architecture foo of test_mfm is

  type CharFile is file of character;

  signal clock50mhz : std_logic := '1';
  signal f_rdata : std_logic := '1';
  -- This is relative to the sample rate of the feed, which in reality will be
  -- 20ns, but in our DMA-captured traces is only 50/3 = ~17MHz, so an interval
  -- should be 4usec * (50/3) = ~67 cycles
  -- This register gets x2 before being used
  signal cycles_per_interval : unsigned(7 downto 0) := to_unsigned(33,8);
  
    -- The track/sector/side we are being asked to find
  signal target_track : unsigned(7 downto 0) := x"01";
  signal target_sector : unsigned(7 downto 0) := x"01";
  signal target_side : unsigned(7 downto 0) := x"01";

    -- Indicate when we have hit the start of the gap leading
    -- to the data area (this is so that sector writing can
    -- begin.  It does have to take account of the latency of
    -- the write stage, and also any write precompensation).
  signal sector_found : std_logic := '0';
  signal sector_data_gap : std_logic := '0';
  signal found_track : unsigned(7 downto 0) := x"00";
  signal found_sector : unsigned(7 downto 0) := x"00";
  signal found_side : unsigned(7 downto 0) := x"00";

    -- Bytes of the sector when reading
  signal first_byte : std_logic := '0';
  signal byte_valid : std_logic := '0';
  signal byte_out : unsigned(7 downto 0);
  signal crc_error : std_logic := '0';
  signal sector_end : std_logic := '0';
  
begin

  decoder0: entity work.mfm_decoder port map (
    clock50mhz => clock50mhz,
    f_rdata => f_rdata,
    cycles_per_interval => cycles_per_interval,
    invalidate => '0',

    target_track => target_track,
    target_sector => target_sector,
    target_side => target_side,

    sector_found => sector_found,
    sector_data_gap => sector_data_gap,
    found_track => found_track,
    found_sector => found_sector,
    found_side => found_side,

    first_byte => first_byte,
    byte_valid => byte_valid,
    byte_out => byte_out,
    crc_error => crc_error,
    sector_end => sector_end
    );
  
  process is
    file trace : CharFile;
    variable c : character;
  begin
    file_open(trace,"assets/track2-40ns.dat",READ_MODE);
    while not endfile(trace) loop
      Read(trace,c);
--      report "Read char $" & to_hstring(to_unsigned(character'pos(c),8));      
      f_rdata <= to_unsigned(character'pos(c),8)(4);
      clock50mhz <= '0';
      wait for 10 ns;
      clock50mhz <= '1';
      wait for 10 ns;
    end loop;
  end process;

  process (clock50mhz,byte_out) is
  begin
    if rising_edge(clock50mhz) then
      if byte_valid='1' then
        report "Read sector byte $" & to_hstring(byte_out)
          & " (first=" & std_logic'image(first_byte)
          & ")";
      end if;
      if (sector_end or crc_error)='1' then
        report "End of sector reached: crc_error="
          & std_logic'image(crc_error);        
      end if;
    end if;
  end process;
  
end foo;