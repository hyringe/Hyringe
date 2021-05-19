package hyperv.general;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Comparator;
import java.util.Iterator;
import java.util.List;
import java.util.Spliterators;
import java.util.stream.Collectors;
import java.util.stream.StreamSupport;

public class HyperVHypercallDict {

    private HyperVHypercallDict(){}

    private static JSONObject jo;

    static {
        try {
            InputStream i = HyperVHypercallDict.class.getClassLoader().getResourceAsStream("hyperv/hypercall_dict.json");
            BufferedReader br = new BufferedReader(new InputStreamReader(i));
            jo = new JSONObject(br.lines().collect(Collectors.joining()));
        } catch (NullPointerException e) {
            throw new IllegalArgumentException("Could not find file \"hyperv/hypercall_dict.json\"");
        } catch (JSONException e) {
            throw new IllegalArgumentException("\"hyperv/hypercall_dict.json\" cannot be parsed");
        }
    }

    private static JSONObject getHypercall(String hypercallName) {
        try {
            return jo.getJSONObject(hypercallName);
        } catch (JSONException e) {
            throw new IllegalArgumentException("\"" + hypercallName + "\" does not exist in dict");
        }
    }

    public static int getCallCode(String hypercallName) {
        try {
            return getHypercall(hypercallName).getInt("call_code");
        } catch (JSONException e) {
            throw new IllegalArgumentException("\"" + hypercallName + "\" has no call code defined.");
        }
    }

    public static boolean isFastcall(String hypercallName) {
        try {
            return getHypercall(hypercallName).getBoolean("fast");
        } catch (JSONException e) {
            return false;
        }
    }

    public static boolean isRepcall(String hypercallName) {
        try {
            return getHypercall(hypercallName).getBoolean("rep");
        } catch (JSONException e) {
            return false;
        }
    }

    private static JSONObject getParameter(String hypercallName, String paramName) {
        try {
            JSONObject j = getHypercall(hypercallName).getJSONObject("params");
            try {
                return j.getJSONObject(paramName);
            } catch (JSONException e) {
                throw new IllegalArgumentException("Parameter \"" + paramName + "\" is not defined for call \"" + hypercallName + "\"");
            }
        } catch (JSONException e) {
            throw new IllegalArgumentException("\"" + hypercallName + "\" does not have parameters defined.");
        }
    }

    private static JSONObject getOutputval(String hypercallName, String outputName) {
        try {
            JSONObject j = getHypercall(hypercallName).getJSONObject("output");
            try {
                return j.getJSONObject(outputName);
            } catch (JSONException e) {
                throw new IllegalArgumentException("Output value \"" + outputName + "\" is not defined for call \"" + hypercallName + "\"");
            }
        } catch (JSONException e) {
            throw new IllegalArgumentException("\"" + hypercallName + "\" does not have output values defined.");
        }
    }

    public static int getParameterSize(String hypercallName, String paramName) {
        try {
            return getParameter(hypercallName, paramName).getInt("size");
        } catch (JSONException e) {
            throw new IllegalArgumentException("Parameter \"" + paramName + "\" of call \"" + hypercallName + "\" has no size defined");
        }
    }

    public static int getParameterOffset(String hypercallName, String paramName) {
        try {
            return getParameter(hypercallName, paramName).getInt("offset");
        } catch (JSONException e) {
            throw new IllegalArgumentException("Parameter \"" + paramName + "\" of call \"" + hypercallName + "\" has no offset defined");
        }
    }

    public static int getOutputvalSize(String hypercallName, String outputvalName) {
        try {
            return getOutputval(hypercallName, outputvalName).getInt("size");
        } catch (JSONException e) {
            throw new IllegalArgumentException("Output value \"" + outputvalName + "\" of call \"" + hypercallName + "\" has no size defined");
        }
    }

    public static int getOutputvalOffset(String hypercallName, String outputvalName) {
        try {
            return getOutputval(hypercallName, outputvalName).getInt("offset");
        } catch (JSONException e) {
            throw new IllegalArgumentException("Output value \"" + outputvalName + "\" of call \"" + hypercallName + "\" has no offset defined");
        }
    }

    public static int getParameterCount(String hypercallName) {
        try {
            JSONObject j = getHypercall(hypercallName).getJSONObject("params");
            return (int) StreamSupport.stream(Spliterators.spliteratorUnknownSize(j.keys(), 0), false).count();
        } catch (JSONException e) {
            throw new IllegalArgumentException("Hypercall \"" + hypercallName + "\" has no parameter section defined");
        }
    }

    public static List<String> getAllParameters(String hypercallName) {
        try {
            JSONObject j = getHypercall(hypercallName).getJSONObject("params");
            Iterator<String> i = j.keys();
            return (List<String>) StreamSupport.stream(Spliterators.spliteratorUnknownSize(j.keys(), 0), false).map(o -> (String) o).collect(Collectors.toList());
        } catch (JSONException e) {
            throw new IllegalArgumentException("Hypercall \"" + hypercallName + "\" has no parameter section defined");
        }
    }

    public static int getRequiredInputPageSize(String hypercallName) {
        return getAllParameters(hypercallName).stream().max(Comparator.comparing(p -> getParameterOffset(hypercallName, p))).map(p -> getParameterOffset(hypercallName, p) + getParameterSize(hypercallName, p)).orElse(0);
    }
}
