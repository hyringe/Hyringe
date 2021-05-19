package hyperv.report;

public class LogFlagDecoder {

    public final boolean exectime, timesteps, result, output;

    public LogFlagDecoder(long flags) {
            exectime = ((flags & 0b1) != 0);
            timesteps = ((flags & 0b10) != 0);
            result = ((flags & 0b1000) != 0);
            output = ((flags & 0b10000) != 0);
    }

    public boolean anyLogAvailable() {
        return exectime || timesteps || result || output;
    }
}
