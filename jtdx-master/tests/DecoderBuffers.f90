program decoder_buffers_test
  use decode_buffers
  implicit none
  integer count,dtcount,i
  character(37) messages(200)
  integer snrs(200)
  real freqs(200),dt(200)
  count=0;dtcount=0;messages='';snrs=0;freqs=0.;dt=0.
!$omp parallel do num_threads(8)
  do i=1,10000
    call store_decode(count,messages,snrs,freqs,'CQ K1ABC FN20',-12,1500.)
    call record_dt(dtcount,dt,0.25)
  enddo
!$omp end parallel do
  if(count.ne.200 .or. dtcount.ne.200) stop 1
  if(any(messages.ne.'CQ K1ABC FN20')) stop 2
  if(any(snrs.ne.-12) .or. any(freqs.ne.1500.) .or. any(dt.ne.0.25)) stop 3
  print *, 'Decoder buffers: 10000 concurrent insertions bounded to 200 passed'
end program
