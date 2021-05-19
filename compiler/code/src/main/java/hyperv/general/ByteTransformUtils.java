package hyperv.general;

import com.google.common.primitives.Longs;
import org.apache.commons.lang3.ArrayUtils;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;
import java.util.List;

public class ByteTransformUtils {

    private ByteTransformUtils(){}

    public static byte[] longToBytes(long val, int size) {
        if (size <= 0 || size > 8)
            throw new IllegalArgumentException("Byte array size has to be in [1,8]");

        return ArrayUtils.subarray(ByteBuffer.allocate(Long.SIZE / Byte.SIZE).order(ByteOrder.LITTLE_ENDIAN).putLong(val).array(), 0, size);
    }

    public static List<Byte> longToByteList(long val, int size) {
        return Arrays.asList(ArrayUtils.toObject(ArrayUtils.subarray(ByteBuffer.allocate(Long.SIZE / Byte.SIZE).order(ByteOrder.LITTLE_ENDIAN).putLong(val).array(), 0, size)));
    }

    public static long bytesToLong(byte[] bytes) {
        ByteBuffer buffer = ByteBuffer.allocate(Longs.BYTES);
        buffer.order(ByteOrder.LITTLE_ENDIAN);
        buffer.put(bytes);
        if (bytes.length < Longs.BYTES)
            buffer.put(new byte[Longs.BYTES - bytes.length]);
        buffer.flip();
        return buffer.getLong();
    }
}
