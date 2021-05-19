package hycallparser.preprocessor;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

public class CampaignFileLoader {

    public static String fromFile(String filename) throws IOException {
        return loadRecursively(filename, new HashMap<>());
    }

    private static String loadRecursively(String filename, Map<String, String> alreadyIncluded) throws IOException {
        Matcher m = Pattern.compile("(.*/)?([^/]+)").matcher(filename);
        m.matches();
        String basepath = m.group(1);
        String file = m.group(2);

        String fileContent = Files.lines(Paths.get(filename)).collect(Collectors.joining("\n"));
        Pattern p = Pattern.compile("include\\((.*)\\)");
        while ((m = p.matcher(fileContent)).find()) {
            String includeStatement = m.group(0);
            String includeFile = m.group(1);
            String includeFileFullPath = basepath + includeFile;

            if (alreadyIncluded.containsKey(includeFileFullPath))
                throw new IOException("File \"" + includeFileFullPath + "\" is included twice, by \"" + alreadyIncluded.get(includeFileFullPath) + "\" and \"" + filename + "\"");
            alreadyIncluded.put(includeFileFullPath, filename);

            String replacementText = loadRecursively(includeFileFullPath, alreadyIncluded);
            fileContent = fileContent.replace(includeStatement, replacementText);
        }

        return fileContent;
    }
}
