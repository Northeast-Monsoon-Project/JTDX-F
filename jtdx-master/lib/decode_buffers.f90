! Fluxmancer: bounded shared decoder statistics, GPLv3.
module decode_buffers
  implicit none
contains
  subroutine store_decode(count,messages,snrs,freqs,message,snr,freq)
    integer, intent(inout) :: count
    character(*), intent(inout) :: messages(:)
    integer, intent(inout) :: snrs(:)
    real, intent(inout) :: freqs(:)
    character(*), intent(in) :: message
    integer, intent(in) :: snr
    real, intent(in) :: freq
!$omp critical(decode_buffer_store)
    if(count.ge.0 .and. count.lt.min(size(messages),size(snrs),size(freqs))) then
      count=count+1
      messages(count)=message
      snrs(count)=snr
      freqs(count)=freq
    endif
!$omp end critical(decode_buffer_store)
  end subroutine
  subroutine record_dt(count,values,dt)
    integer, intent(inout) :: count
    real, intent(inout) :: values(:)
    real, intent(in) :: dt
!$omp critical(decode_dt_store)
    if(count.ge.0 .and. count.lt.size(values)) then
      count=count+1
      values(count)=dt
    endif
!$omp end critical(decode_dt_store)
  end subroutine
end module
