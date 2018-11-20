# DigSig
Digital Signal Analysis Code
DigSig - base digital signal class
DigAna - uses DigSig to do analysis on data

Procedure for working with any pulses

1. Calculate ped (digsig_calcped.C).
   This creates a ped file in
      PED/xxx_ped.txt
   The pedestal is by default ped0, which is the straight mean and rms of the
   distribution.  You have to set the x-range (either samplenum or time) where
   there is a good pedestal region.

2. Make the template waveforms for the fits (digsig_maketemplate.C)
   The bootstrap (pass 0) uses a spline fit to the data points to normalize
   the amplitude and start time, and generates a template waveform normalized
   to an amplitude of 1 and same start time for all pulses.
   The next pass 1 uses the template from the spline for a chi^2 fit to
   generate the normalized waveform.
   The next pass 2 uses the pass1 template for a chi^2 fit... and so on.

3. Any analysis can now use the templates by loading up the correct pedestal
   and the correct template
       digana.SetPed0FromFile(pedfile);
       digana.ReadTemplate(name);
   An example is digsig_calctimes.C
   Note that you can also skip step 2 if you use a dCFD method,
   by setting time_method = 0 in digsig_calctimes.C


